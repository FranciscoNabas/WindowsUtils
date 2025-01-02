using System.Runtime.InteropServices;
using System.Text;

namespace WindowsUtils.Engine.Interop;

internal enum HeapAllocFlags : uint
{
    NONE                 = 0x00000000,
    NO_SERIALIZE         = 0x00000001,
    GENERATE_EXCEPTIONS  = 0x00000004,
    ZERO_MEMORY          = 0x00000008,
}

internal static class NativeHeap
{
    private static IntPtr m_processHeap;

    internal static IntPtr ProcessHeap {
        get {
            if (m_processHeap == IntPtr.Zero)
                m_processHeap = GetProcessHeap();

            if (m_processHeap == IntPtr.Zero)
                throw new NativeException(Marshal.GetLastWin32Error());

            return m_processHeap;
        }
    }

    [DllImport("kernel32.dll")]
    private static extern IntPtr HeapAlloc(
        IntPtr hHeap,
        HeapAllocFlags dwFlags,
        ulong dwBytes
    );

    [DllImport("kernel32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool HeapFree(
        IntPtr hHeap,
        HeapAllocFlags dwFlags,
        IntPtr lpMem
    );

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr GetProcessHeap();

    internal static IntPtr Alloc(int size)
        => Alloc((ulong)size);

    internal static IntPtr Alloc(ulong size)
    {
        IntPtr mem = HeapAlloc(ProcessHeap, HeapAllocFlags.ZERO_MEMORY, size);
        if (mem == IntPtr.Zero)
            throw new OutOfMemoryException();

        return mem;
    }

    internal static void Free(IntPtr mem)
        => HeapFree(ProcessHeap, 0, mem);

    internal static unsafe IntPtr StringToHeapAllocUni(string str)
    {
        if (str is null)
            return IntPtr.Zero;

        long buffSize = (str.Length + 1) * 2;
        IntPtr mem = HeapAlloc(ProcessHeap, HeapAllocFlags.NONE, (ulong)buffSize);
        fixed (char* strPtr = str) {
            Buffer.MemoryCopy(strPtr, mem.ToPointer(), buffSize, buffSize - 2);
        }

        return mem;
    }

    internal static unsafe IntPtr StringToHeapAllocAnsi(string str)
    {
        if (str is null)
            return IntPtr.Zero;

        long buffSize = str.Length + 1;
        IntPtr mem = HeapAlloc(ProcessHeap, HeapAllocFlags.ZERO_MEMORY, (ulong)buffSize);
        fixed (byte* ansiPtr = Encoding.ASCII.GetBytes(str))
            Buffer.MemoryCopy(ansiPtr, mem.ToPointer(), buffSize, buffSize - 1);

        return mem;
    }
}