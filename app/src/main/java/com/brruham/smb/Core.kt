package com.brruham.smb

object Core {
    init { System.loadLibrary("smbcore") }
    @Volatile var ready = false
    external fun init(romPath: String): Int
    external fun size(): Int
    external fun input(mask: Int)
    external fun frame(out: IntArray)
    external fun audio(out: FloatArray, n: Int)
}
