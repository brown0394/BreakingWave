# Generates the grey-box beach heightmap for BreakingWave.
# Output: 16-bit grayscale PNG, 1009x1009, for UE landscape import at scale X=100 Y=100 Z=100.
# With that scale: 1 pixel = 1 m, gray value 32768 = Z 0 (waterline), 128 gray steps = 1 m of height.
#
# Beach profile along image rows (top of image = sea, bottom = high ground; advance direction = +Y):
#   rows    0-250   sea floor          -8.0 m -> -1.5 m
#   rows  250-310   Zone 0 shallows    -1.5 m ->  0.0 m
#   rows  310-390   Zone 1 kill zone    0.0 m -> +2.0 m   (kept almost perfectly flat)
#   rows  390-480   Zone 2 obstacles   +2.0 m -> +6.0 m
#   rows  480-580   Zone 3 upper beach +6.0 m -> +13 m    (dune undulation)
#   rows  580-660   Zone 4 defense     +13 m  -> +24 m
#   rows  660-730   bluff              +24 m  -> +32 m
#   rows  730-1008  high ground        +32 m  plateau
# All numbers tentative per CLAUDE.md - edit the Profile table below and re-run.

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
        {    0, -8.0, 0.30 },
        {  250, -1.5, 0.15 },
        {  310,  0.0, 0.05 },
        {  390,  2.0, 0.08 },
        {  480,  6.0, 0.60 },
        {  580, 13.0, 0.90 },
        {  660, 24.0, 0.50 },
        {  730, 32.0, 0.70 },
        { 1008, 32.5, 0.70 }
    };

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

    public static void Generate(string path)
    {
        ushort[] pixels = new ushort[Size * Size];
        for (int row = 0; row < Size; row++)
        {
            double baseHeight = Hermite(row, 1);
            double amplitude = Hermite(row, 2);
            for (int col = 0; col < Size; col++)
            {
                double meters = baseHeight + Noise(col, row) * amplitude;
                double value = 32768.0 + meters * 128.0;
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
