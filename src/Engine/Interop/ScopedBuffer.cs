namespace WindowsUtils.Engine.Interop;

/// <summary>
/// A disposable unmanaged memory buffer.
/// </summary>
/// <remarks>
/// This is used throughout the native APIs so we can easily allocate
/// memory without having to explicitly free it (while using the disposable pattern).
/// </remarks>
internal sealed class ScopedBuffer : IDisposable
{
    private IntPtr m_buffer;
    private bool m_isDisposed;
    private ulong m_size;

#if DEBUG
    private Guid m_id;

    internal Guid Id => m_id;
#endif

    internal ulong Size => m_size;

    internal ScopedBuffer(int size)
        : this((ulong)size) { }

    internal ScopedBuffer(long size)
        : this((ulong)size) { }

    /// <summary>
    /// Allocates a buffer of the specified size.
    /// </summary>
    /// <param name="size">The buffer size.</param>
    internal ScopedBuffer(ulong size)
    {
        m_buffer = NativeHeap.Alloc(size);
        m_size = size;
        m_isDisposed = false;

#if DEBUG
        m_id = Guid.NewGuid();
#endif
    }

    /// <summary>
    /// Allocates a buffer from a managed string.
    /// </summary>
    /// <param name="str">The input string.</param>
    /// <param name="ansi">True if the string is ANSI.</param>
    internal ScopedBuffer(string str, bool ansi = false)
    {
        if (ansi) {
            m_buffer = NativeHeap.StringToHeapAllocAnsi(str);
            m_size = (ulong)str.Length;
            m_isDisposed = false;
        }
        else {
            m_buffer = NativeHeap.StringToHeapAllocUni(str);
            m_size = (ulong)str.Length * 2;
            m_isDisposed = false;
        }

#if DEBUG
        m_id = Guid.NewGuid();
#endif
    }

    ~ScopedBuffer()
        => Dispose(disposing: false);

    /// <summary>
    /// Resizes the current buffer.
    /// </summary>
    /// <param name="newSize">The new size.</param>
    internal void Resize(ulong newSize)
    {
        if (m_isDisposed)
            throw new ObjectDisposedException("ScopedBuffer");

        NativeHeap.Free(m_buffer);

        m_buffer = NativeHeap.Alloc(newSize);
        m_size = newSize;
    }

    /// <summary>
    /// Frees the buffer memory.
    /// </summary>
    public void Dispose()
    {
        Dispose(disposing: true);
        GC.SuppressFinalize(this);
    }

    /// <summary>
    /// Frees the buffer memory.
    /// </summary>
    /// <param name="disposing">True to free the buffer memory.</param>
    private void Dispose(bool disposing)
    {
        if (disposing && !m_isDisposed) {
            NativeHeap.Free(m_buffer);

            m_size = 0;
            m_buffer = IntPtr.Zero;
            m_isDisposed = true;
        }
    }

    // Operators to make our lives easier (sometimes harder).
    public static implicit operator IntPtr(ScopedBuffer other)
    {
        if (other.m_isDisposed)
            throw new ObjectDisposedException(nameof(other));

        return other.m_buffer;
    }

    public static unsafe implicit operator void*(ScopedBuffer other)
    {
        if (other.m_isDisposed)
            throw new ObjectDisposedException(nameof(other));

        return other.m_buffer.ToPointer();
    }
}