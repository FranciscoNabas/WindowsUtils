using System;
using System.Collections.Generic;
using WindowsUtils;
using Wrapper;

namespace TestExec
{
    class Program
    {
        static void Main(string[] args)
        {
            List<Managed.wSessionEnumOutput> thing = Utilities.GetComputerSession("USFLORLCMCP01P");
        }
    }
}