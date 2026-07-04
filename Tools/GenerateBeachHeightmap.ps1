# Generates the grey-box beach heightmap for BreakingWave.
# Output: 16-bit grayscale PNG, 1009x1009, for UE landscape import at scale X=100 Y=100 Z=200.
# With that scale: 1 pixel = 1 m, gray value 32768 = Z 0 (waterline), 64 gray steps = 1 m of height.
# (Z=200 chosen after walkthrough - profile-true heights at Z=100 read as flat in first person.)
#
# Beach profile along image rows (top of image = sea, bottom = high ground; advance direction = +Y):
#   rows    0-250   sea floor          -16 m -> -3.0 m
#   rows  250-310   Zone 0 shallows    -3.0 m ->  0.0 m
#   rows  310-390   Zone 1 kill zone    0.0 m -> +4.0 m   (kept almost perfectly flat)
#   rows  390-480   Zone 2 obstacles   +4.0 m -> +12 m
#   rows  480-580   Zone 3 upper beach +12 m  -> +26 m    (3 sightline-blocking dunes + undulation)
#   rows  580-660   Zone 4 defense     +26 m  -> +48 m
#   rows  660-730   bluff              +48 m  -> +64 m
#   rows  730-1008  high ground        +64 m  plateau
# Tactical relief on top of the profile:
#   - 3 dunes in Zone 3 (Dunes table), tall enough to block sightlines while prone/crouched
#   - shingle berm ridge with gaps at the Zone 2/3 boundary (Berm constants)
#   - bluff line wavers laterally +/-15 m from Zone 3 inland (BluffWaviness)
#   - shell craters with raised rims, mostly Zone 1 (Craters table): 3 deep ones are the only
#     cover in the kill zone (scarce by design), ~10 shallow ones are bombardment dressing
# All numbers tentative per CLAUDE.md - edit the tables/constants below and re-run.

param([string]$OutputPath = (Join-Path (Split-Path $PSScriptRoot -Parent) "SourceAssets\BeachHeightmap_1009.png"))

$source = @'
using System;
using System.IO;
using System.IO.Compression;

public static class BeachHeightmap
{
    const int Size = 1009;

    // { row (m from sea edge), terrain height (m), lateral noise amplitude (m) }
    static readonly double[,] Profile = {
        {    0, -16.0, 0.60 },
        {  250,  -3.0, 0.30 },
        {  310,   0.0, 0.10 },
        {  390,   4.0, 0.16 },
        {  480,  12.0, 1.20 },
        {  580,  26.0, 1.80 },
        {  660,  48.0, 1.00 },
        {  730,  64.0, 1.40 },
        { 1008,  65.0, 1.40 }
    };

    // { center col (m), center row (m), height (m), along-shore radius (m), inland radius (m) }
    static readonly double[,] Dunes = {
        { 250, 525, 5.2, 26, 10 },
        { 510, 548, 6.0, 30, 12 },
        { 770, 518, 4.4, 22,  9 }
    };

    // Shell craters, mostly Zone 1 (rows 310-390), a few bleeding into Zone 0/2 edges.
    // Deep ones (>= ~1.5 m) are usable cover and stay SCARCE by design - Zone 1 is the kill
    // zone. Shallow ones (< ~1 m) are bombardment dressing, no protection from plunging fire.
    // Each crater gets a raised rim (see CraterRim* constants below).
    // { center col (m), center row (m), depth (m), along-shore radius (m), inland radius (m) }
    static readonly double[,] Craters = {
        { 150, 372, -1.6, 5.5, 4.5 },
        { 430, 348, -1.8, 6.0, 5.0 },
        { 610, 366, -2.0, 7.0, 5.5 },
        {  80, 330, -0.6, 4.0, 3.5 },
        { 200, 302, -0.6, 4.5, 3.8 },
        { 260, 358, -0.8, 5.0, 4.0 },
        { 350, 322, -0.5, 3.5, 3.0 },
        { 505, 340, -0.7, 4.5, 3.8 },
        { 560, 398, -0.8, 4.8, 4.0 },
        { 700, 328, -0.6, 4.0, 3.2 },
        { 760, 375, -0.9, 5.5, 4.5 },
        { 880, 350, -0.7, 4.2, 3.6 },
        { 940, 318, -0.5, 3.6, 3.0 }
    };

    const double CraterRimRadius = 2.0;
    const double CraterRimWidth = 0.5;
    const double CraterRimHeightRatio = 0.3;

    const double BermRow = 478;
    const double BermMaxHeight = 2.8;
    const double BermThickness = 5.0;

    const double BluffWavinessAmplitude = 15.0;
    const double BluffWarpStartRow = 450;
    const double BluffWarpFullRow = 580;

    static double Hermite(double y, int column)
    {
        int last = Profile.GetLength(0) - 1;
        if (y <= Profile[0, 0]) return Profile[0, column];
        if (y >= Profile[last, 0]) return Profile[last, column];

        int i = 0;
        while (y > Profile[i + 1, 0]) i++;

        double r1 = Profile[i, 0], r2 = Profile[i + 1, 0];
        double h1 = Profile[i, column], h2 = Profile[i + 1, column];
        double m1 = SlopeAt(i, column);
        double m2 = SlopeAt(i + 1, column);
        double dt = r2 - r1;
        double t = (y - r1) / dt;
        double t2 = t * t, t3 = t2 * t;

        return (2 * t3 - 3 * t2 + 1) * h1
             + (t3 - 2 * t2 + t) * dt * m1
             + (-2 * t3 + 3 * t2) * h2
             + (t3 - t2) * dt * m2;
    }

    static double SlopeAt(int i, int column)
    {
        int last = Profile.GetLength(0) - 1;
        int a = i > 0 ? i - 1 : i;
        int b = i < last ? i + 1 : i;
        return (Profile[b, column] - Profile[a, column]) / (Profile[b, 0] - Profile[a, 0]);
    }

    static double Noise(double x, double y)
    {
        return Math.Sin(x * 0.0093 + 1.7) * Math.Sin(y * 0.0081 + 0.4) * 0.55
             + Math.Sin(x * 0.0310 + 0.9) * Math.Sin(y * 0.0270 + 2.1) * 0.30
             + Math.Sin(x * 0.1100 + 2.3) * Math.Sin(y * 0.0970 + 1.2) * 0.15;
    }

    static double BluffWaviness(double x)
    {
        return Math.Sin(x * 0.0071 + 0.8) * 0.6
             + Math.Sin(x * 0.0023 + 2.4) * 0.4;
    }

    static double BluffWarpWeight(double y)
    {
        if (y <= BluffWarpStartRow) return 0;
        if (y >= BluffWarpFullRow) return 1;
        double t = (y - BluffWarpStartRow) / (BluffWarpFullRow - BluffWarpStartRow);
        return t * t * (3 - 2 * t);
    }

    static double BermHeight(double x, double y)
    {
        double envelope = 0.55 + Math.Sin(x * 0.021 + 1.3) * 0.45
                               + Math.Sin(x * 0.0049 + 0.6) * 0.35;
        if (envelope < 0) envelope = 0;
        if (envelope > 1) envelope = 1;
        double d = y - BermRow;
        return BermMaxHeight * envelope * Math.Exp(-(d * d) / (2 * BermThickness * BermThickness));
    }

    static double DuneHeight(double x, double y)
    {
        double sum = 0;
        for (int i = 0; i < Dunes.GetLength(0); i++)
        {
            double dx = (x - Dunes[i, 0]) / Dunes[i, 3];
            double dy = (y - Dunes[i, 1]) / Dunes[i, 4];
            sum += Dunes[i, 2] * Math.Exp(-(dx * dx + dy * dy) * 0.5);
        }
        return sum;
    }

    static double CraterHeight(double x, double y)
    {
        double sum = 0;
        for (int i = 0; i < Craters.GetLength(0); i++)
        {
            double dx = (x - Craters[i, 0]) / Craters[i, 3];
            double dy = (y - Craters[i, 1]) / Craters[i, 4];
            double r = Math.Sqrt(dx * dx + dy * dy);
            double depth = Craters[i, 2];
            double bowl = depth * Math.Exp(-r * r * 0.5);
            double rimOffset = r - CraterRimRadius;
            double rim = -depth * CraterRimHeightRatio
                       * Math.Exp(-(rimOffset * rimOffset) / (2 * CraterRimWidth * CraterRimWidth));
            sum += bowl + rim;
        }
        return sum;
    }

    public static void Generate(string path)
    {
        ushort[] pixels = new ushort[Size * Size];
        for (int row = 0; row < Size; row++)
        {
            double warpWeight = BluffWarpWeight(row);
            for (int col = 0; col < Size; col++)
            {
                double warpedRow = row + BluffWaviness(col) * BluffWavinessAmplitude * warpWeight;
                double baseHeight = Hermite(warpedRow, 1);
                double amplitude = Hermite(warpedRow, 2);
                double meters = baseHeight
                              + Noise(col, row) * amplitude
                              + BermHeight(col, row)
                              + DuneHeight(col, row)
                              + CraterHeight(col, row);
                double value = 32768.0 + meters * 64.0;
                if (value < 0) value = 0;
                if (value > 65535) value = 65535;
                pixels[row * Size + col] = (ushort)value;
            }
        }
        Directory.CreateDirectory(Path.GetDirectoryName(path));
        WritePng(path, pixels, Size, Size);
    }

    static void WritePng(string path, ushort[] pixels, int width, int height)
    {
        byte[] raw = new byte[height * (1 + width * 2)];
        int p = 0;
        for (int row = 0; row < height; row++)
        {
            raw[p++] = 0;
            for (int col = 0; col < width; col++)
            {
                ushort v = pixels[row * width + col];
                raw[p++] = (byte)(v >> 8);
                raw[p++] = (byte)(v & 0xFF);
            }
        }

        byte[] ihdr = new byte[13];
        WriteBE(ihdr, 0, (uint)width);
        WriteBE(ihdr, 4, (uint)height);
        ihdr[8] = 16; ihdr[9] = 0; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;

        byte[] idat;
        using (MemoryStream ms = new MemoryStream())
        {
            ms.WriteByte(0x78); ms.WriteByte(0x9C);
            using (DeflateStream ds = new DeflateStream(ms, CompressionMode.Compress, true))
                ds.Write(raw, 0, raw.Length);
            uint adler = Adler32(raw);
            byte[] a = new byte[4];
            WriteBE(a, 0, adler);
            ms.Write(a, 0, 4);
            idat = ms.ToArray();
        }

        using (FileStream fs = new FileStream(path, FileMode.Create))
        {
            byte[] sig = { 137, 80, 78, 71, 13, 10, 26, 10 };
            fs.Write(sig, 0, sig.Length);
            WriteChunk(fs, "IHDR", ihdr);
            WriteChunk(fs, "IDAT", idat);
            WriteChunk(fs, "IEND", new byte[0]);
        }
    }

    static void WriteChunk(Stream s, string type, byte[] data)
    {
        byte[] len = new byte[4];
        WriteBE(len, 0, (uint)data.Length);
        s.Write(len, 0, 4);
        byte[] body = new byte[4 + data.Length];
        for (int i = 0; i < 4; i++) body[i] = (byte)type[i];
        Array.Copy(data, 0, body, 4, data.Length);
        s.Write(body, 0, body.Length);
        byte[] crc = new byte[4];
        WriteBE(crc, 0, Crc32(body));
        s.Write(crc, 0, 4);
    }

    static void WriteBE(byte[] buf, int offset, uint v)
    {
        buf[offset] = (byte)(v >> 24);
        buf[offset + 1] = (byte)(v >> 16);
        buf[offset + 2] = (byte)(v >> 8);
        buf[offset + 3] = (byte)v;
    }

    static uint[] crcTable;

    static uint Crc32(byte[] data)
    {
        if (crcTable == null)
        {
            crcTable = new uint[256];
            for (uint n = 0; n < 256; n++)
            {
                uint c = n;
                for (int k = 0; k < 8; k++)
                    c = (c & 1) != 0 ? 0xEDB88320 ^ (c >> 1) : c >> 1;
                crcTable[n] = c;
            }
        }
        uint crc = 0xFFFFFFFF;
        foreach (byte b in data)
            crc = crcTable[(crc ^ b) & 0xFF] ^ (crc >> 8);
        return crc ^ 0xFFFFFFFF;
    }

    static uint Adler32(byte[] data)
    {
        uint a = 1, b = 0;
        foreach (byte d in data)
        {
            a = (a + d) % 65521;
            b = (b + a) % 65521;
        }
        return (b << 16) | a;
    }
}
'@

Add-Type -TypeDefinition $source -Language CSharp
[BeachHeightmap]::Generate($OutputPath)
Write-Host "Heightmap written to $OutputPath"
