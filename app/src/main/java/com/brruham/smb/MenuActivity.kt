package com.brruham.smb

import android.app.Activity
import android.content.Intent
import android.graphics.Color
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.text.InputType
import android.view.Gravity
import android.view.ViewGroup
import android.view.WindowManager
import android.widget.Button
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.TextView
import java.net.Inet4Address
import java.net.NetworkInterface

class MenuActivity : Activity() {
    private val h = Handler(Looper.getMainLooper())
    private lateinit var root: LinearLayout
    private var polling = false

    override fun onCreate(b: Bundle?) {
        super.onCreate(b)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER
            setBackgroundColor(Color.BLACK)
            setPadding(48, 48, 48, 48)
        }
        setContentView(root)
        showMain()
    }

    private fun lp() = LinearLayout.LayoutParams(
        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT
    ).apply { setMargins(0, 16, 0, 16) }

    private fun btn(t: String, f: () -> Unit) {
        val v = Button(this).apply { text = t; textSize = 20f; setOnClickListener { f() } }
        root.addView(v, lp())
    }

    private fun txt(t: String) {
        val v = TextView(this).apply { text = t; textSize = 18f; setTextColor(Color.WHITE); gravity = Gravity.CENTER }
        root.addView(v, lp())
    }

    private fun clear() { root.removeAllViews(); polling = false }

    private fun ips(): String {
        val r = ArrayList<String>()
        try {
            for (n in NetworkInterface.getNetworkInterfaces()) {
                if (n.isLoopback || !n.isUp) continue
                for (a in n.inetAddresses) if (a is Inet4Address) r.add(a.hostAddress ?: "")
            }
        } catch (e: Exception) {}
        return if (r.isEmpty()) "(tidak ada jaringan)" else r.joinToString(", ")
    }

    private fun showMain() {
        clear()
        txt("SUPER MARIO BROS")
        btn("Single Player") { startGame(false) }
        btn("Multiplayer") { showMulti() }
    }

    private fun showMulti() {
        clear()
        Core.netStop()
        txt("Multiplayer")
        btn("Host (Mario)") { showHost() }
        btn("Join (Luigi)") { showJoin() }
        btn("Kembali") { showMain() }
    }

    private fun showHost() {
        clear()
        Core.netStop()
        Core.netHost()
        txt("Menunggu pemain...\nIP: ${ips()}\nPort: 7777")
        btn("Batal") { Core.netStop(); showMulti() }
        poll()
    }

    private fun showJoin() {
        clear()
        txt("IP host:")
        val et = EditText(this).apply {
            hint = "192.168.43.1"
            setTextColor(Color.WHITE)
            setHintTextColor(Color.GRAY)
            inputType = InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_VARIATION_URI
        }
        root.addView(et, lp())
        btn("Connect") {
            val ip = et.text.toString().trim()
            if (ip.isNotEmpty()) {
                clear()
                txt("Menghubungkan ke $ip ...")
                btn("Batal") { Core.netStop(); showMulti() }
                Core.netStop()
                Core.netJoin(ip)
                poll()
            }
        }
        btn("Kembali") { showMulti() }
    }

    private fun poll() {
        polling = true
        h.postDelayed(object : Runnable {
            override fun run() {
                if (!polling) return
                when (Core.netState()) {
                    2 -> { polling = false; startGame(true) }
                    3 -> {
                        clear()
                        txt("Koneksi gagal")
                        btn("Kembali") { showMulti() }
                    }
                    else -> h.postDelayed(this, 250)
                }
            }
        }, 250)
    }

    private fun startGame(mp: Boolean) {
        startActivity(Intent(this, MainActivity::class.java).putExtra("mp", mp))
        finish()
    }
}
