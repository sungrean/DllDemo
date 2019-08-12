using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace ConsoleApp1
{
    class Program
    {
        //[DllImport("C++_CScharp_DLL.dll")]
        // private static extern string getResult(byte[] a); 
        [DllImport("Project1.dll", CharSet = CharSet.Auto, CallingConvention = CallingConvention.Cdecl)]
        static extern void getResult(ref byte buf, ref byte result, int size);
        //[MarshalAs(UnmanagedType.LPArray)] 

        static void Main(string[] args)
        {
            //Console.WriteLine(fun.say("Hello World"));
            byte[] b = { (byte)'H', (byte)'e', (byte)'l', (byte)'l', (byte)'o', (byte)'!' };
            byte[] result = new byte[4096];
            getResult(ref b[0], ref result[0], b.Length);

            Console.WriteLine(System.Text.Encoding.Default.GetString(result));
            Console.ReadKey();

        }

    }
}
