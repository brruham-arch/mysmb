package com.brruham.smb

import android.app.Activity
import android.os.Bundle
import android.view.WindowManager
import java.io.File

class MainActivity : Activity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        if (!Core.ready) {
            val rom = File(filesDir, "smb.nes")
            if (!rom.exists() || rom.length() == 0L) {
                assets.open("smb.nes").use { i -> rom.outputStream().use { o -> i.copyTo(o) } }
            }
            Core.init(rom.absolutePath)
            Core.ready = true
        }
        setContentView(GameView(this))
    }
}
