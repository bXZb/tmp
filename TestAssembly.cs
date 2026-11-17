// TestAssembly.cs - IEA probe: file-write marker bypasses the output pipe
using System;
using System.IO;

public class TestAssembly
{
    public static void Main(string[] args)
    {
        File.WriteAllText(@"C:\Users\Public\iea_ran.txt",
            "IEA-CLR-RAN " + DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss") + " args=" + string.Join(",", args));
        Console.WriteLine("IEA-TEST-OK");
    }
}
