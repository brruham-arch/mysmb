package com.brruham.smb

import android.content.Context
import android.graphics.*
import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioTrack
import android.os.Process
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.SurfaceView

class GameView(ctx: Context) : SurfaceView(ctx), SurfaceHolder.Callback {
    private val RIGHT = 0x80; private val LEFT = 0x40; private val DOWN = 0x20; private val UP = 0x10
    private val START = 0x08; private val SELECT = 0x04; private val B = 0x02; private val A = 0x01
    private var tax = 0f; private var tay = 0f; private var tbx = 0f; private var tby = 0f; private var tbr = 0f

    private val sz = Core.size()
    private val sw = sz shr 16
    private val sh = sz and 0xFFFF
    private val pixels = IntArray(sw * sh)
    private val bmp = Bitmap.createBitmap(sw, sh, Bitmap.Config.ARGB_8888)

    @Volatile private var running = false
    @Volatile private var dirty = false
    @Volatile private var mask = 0
    private var gameThread: Thread? = null
    private var drawThread: Thread? = null

    private var gameRect = RectF()
    private var dpx = 0f; private var dpy = 0f; private var dr = 0f
    private var ax = 0f; private var ay = 0f; private var bx = 0f; private var by = 0f; private var br = 0f
    private var selRect = RectF(); private var startRect = RectF()

    private val fill = Paint(Paint.ANTI_ALIAS_FLAG)
    private val line = Paint(Paint.ANTI_ALIAS_FLAG).apply { style = Paint.Style.STROKE; strokeWidth = 4f; color = Color.WHITE; alpha = 120 }
    private val txt = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = Color.WHITE; alpha = 200; textAlign = Paint.Align.CENTER }

    init { holder.addCallback(this) }

    override fun onSizeChanged(w: Int, h: Int, ow: Int, oh: Int) {
        val wf = w.toFloat(); val hf = h.toFloat()
        val gh = wf * sh / sw
        gameRect = RectF(0f, 0f, wf, gh)
        val cy = gh + (hf - gh) * 0.45f
        dpx = wf * 0.24f; dpy = cy; dr = wf * 0.17f
        ax = wf * 0.86f; ay = cy - wf * 0.05f
        bx = wf * 0.66f; by = cy + wf * 0.05f; br = wf * 0.095f
        val ry = hf - (hf - gh) * 0.15f
        selRect = RectF(wf * 0.30f, ry - wf * 0.035f, wf * 0.48f, ry + wf * 0.035f)
        startRect = RectF(wf * 0.52f, ry - wf * 0.035f, wf * 0.70f, ry + wf * 0.035f)
        txt.textSize = wf * 0.04f
        tax = ax; tay = ay - wf * 0.20f; tbx = bx; tby = by + wf * 0.20f; tbr = br * 0.75f
    }

    private fun maskAt(x: Float, y: Float): Int {
        var m = 0
        val dx = x - dpx; val dy = y - dpy
        if (dx * dx + dy * dy < dr * dr * 2.2f) {
            val t = dr * 0.3f
            if (dx > t) m = m or RIGHT
            if (dx < -t) m = m or LEFT
            if (dy > t) m = m or DOWN
            if (dy < -t) m = m or UP
        }
        if ((x - ax) * (x - ax) + (y - ay) * (y - ay) < br * br * 1.5f) m = m or A
        if ((x - bx) * (x - bx) + (y - by) * (y - by) < br * br * 1.5f) m = m or B
        if (selRect.contains(x, y)) m = m or SELECT
        if (startRect.contains(x, y)) m = m or START
        if ((x - tax) * (x - tax) + (y - tay) * (y - tay) < tbr * tbr * 1.5f) m = m or 0x100
        if ((x - tbx) * (x - tbx) + (y - tby) * (y - tby) < tbr * tbr * 1.5f) m = m or 0x200
        return m
    }

    override fun onTouchEvent(e: MotionEvent): Boolean {
        var m = 0
        val act = e.actionMasked
        if (act == MotionEvent.ACTION_DOWN && e.y > gameRect.bottom && e.y < gameRect.bottom + txt.textSize * 2.5f) showDbg = !showDbg
        val ended = act == MotionEvent.ACTION_UP || act == MotionEvent.ACTION_CANCEL
        if (!ended) {
            for (i in 0 until e.pointerCount) {
                if (act == MotionEvent.ACTION_POINTER_UP && i == e.actionIndex) continue
                m = m or maskAt(e.getX(i), e.getY(i))
            }
        }
        mask = m
        Core.input(m)
        return true
    }

    private fun btn(c: Canvas, cx: Float, cy: Float, r: Float, label: String, on: Boolean) {
        fill.color = Color.WHITE; fill.alpha = if (on) 110 else 40
        c.drawCircle(cx, cy, r, fill); c.drawCircle(cx, cy, r, line)
        c.drawText(label, cx, cy + txt.textSize / 3f, txt)
    }

    private fun drawUi(c: Canvas) {
        c.drawColor(Color.BLACK)
        c.drawBitmap(bmp, null, gameRect, null); if (showDbg) c.drawText(dbg, c.width / 2f, gameRect.bottom + txt.textSize * 1.5f, txt)
        val m = mask
        btn(c, dpx, dpy - dr * 0.65f, dr * 0.32f, "^", m and UP != 0)
        btn(c, dpx, dpy + dr * 0.65f, dr * 0.32f, "v", m and DOWN != 0)
        btn(c, dpx - dr * 0.65f, dpy, dr * 0.32f, "<", m and LEFT != 0)
        btn(c, dpx + dr * 0.65f, dpy, dr * 0.32f, ">", m and RIGHT != 0)
        btn(c, ax, ay, br, "A", m and A != 0)
        btn(c, tax, tay, tbr, "TA", m and 0x100 != 0)
        btn(c, tbx, tby, tbr, "TB", m and 0x200 != 0)
        btn(c, bx, by, br, "B", m and B != 0)
        fill.color = Color.WHITE
        fill.alpha = if (m and SELECT != 0) 110 else 40
        c.drawRoundRect(selRect, 20f, 20f, fill); c.drawRoundRect(selRect, 20f, 20f, line)
        c.drawText("SELECT", selRect.centerX(), selRect.centerY() + txt.textSize / 3f, txt)
        fill.alpha = if (m and START != 0) 110 else 40
        c.drawRoundRect(startRect, 20f, 20f, fill); c.drawRoundRect(startRect, 20f, 20f, line)
        c.drawText("START", startRect.centerX(), startRect.centerY() + txt.textSize / 3f, txt)
    }

    private var track: AudioTrack? = null
    private val abuf = FloatArray(800)
    private var renderThread: android.os.HandlerThread? = null
    private var lastT = 0L
    private var acc = 0L
    @Volatile private var dbg = ""
    @Volatile private var showDbg = true
    private var fMax = 0L; private var fSum = 0L; private var fN = 0
    private var aFull = 0; private var tDbg = 0L
    private var pollN = 0
    private var scriptStamp = -1L
    private var permAsked = false
    private fun pollScript() {
        if (android.os.Build.VERSION.SDK_INT >= 30 && !android.os.Environment.isExternalStorageManager()) {
            if (!permAsked) {
                permAsked = true
                try {
                    val i = android.content.Intent(android.provider.Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                        android.net.Uri.parse("package:" + context.packageName))
                    i.addFlags(android.content.Intent.FLAG_ACTIVITY_NEW_TASK)
                    context.startActivity(i)
                } catch (e: Exception) {}
            }
            return
        }
        val f = java.io.File("/sdcard/SMB/main.lua")
        if (f.canRead() && f.lastModified() != scriptStamp) {
            scriptStamp = f.lastModified()
            Core.luaLoad(f.readText())
        }
    }

    private fun makeTrack(): AudioTrack {
        val t = AudioTrack.Builder()
            .setAudioAttributes(AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION).build())
            .setAudioFormat(AudioFormat.Builder()
                .setEncoding(AudioFormat.ENCODING_PCM_FLOAT)
                .setSampleRate(48000)
                .setChannelMask(AudioFormat.CHANNEL_OUT_MONO).build())
            .setBufferSizeInBytes(800 * 4 * 8)
            .setTransferMode(AudioTrack.MODE_STREAM)
            .build()
        val silence = FloatArray(800)
        t.write(silence, 0, 800, AudioTrack.WRITE_NON_BLOCKING)
        t.write(silence, 0, 800, AudioTrack.WRITE_NON_BLOCKING)
        t.play()
        return t
    }

    private fun step() {
        if (++pollN % 60 == 1) pollScript()
        val t0 = System.nanoTime()
        Core.frame(pixels)
        val dt = System.nanoTime() - t0
        if (dt > fMax) fMax = dt
        fSum += dt; fN++
        Core.audio(abuf, 800)
        val w = track?.write(abuf, 0, 800, AudioTrack.WRITE_NON_BLOCKING) ?: 800
        if (w < 800) aFull++
        bmp.setPixels(pixels, 0, sw, 0, 0, sw, sh)
    }

    private val vsync = object : android.view.Choreographer.FrameCallback {
        override fun doFrame(t: Long) {
            if (!running) return
            android.view.Choreographer.getInstance().postFrameCallback(this)
            val frameNs = 16_666_667L
            if (lastT == 0L) lastT = t
            acc += t - lastT; lastT = t
            var steps = 0
            while (acc >= frameNs - 2_000_000L && steps < 2) { acc -= frameNs; steps++; step() }
            if (acc > frameNs) acc = 0
            if (steps == 0) return
            if (t - tDbg > 1_000_000_000L) {
                dbg = Core.luaStatus()
                fMax = 0; fSum = 0; fN = 0; aFull = 0; tDbg = t
            }
            val c = if (android.os.Build.VERSION.SDK_INT >= 26) holder.lockHardwareCanvas() else holder.lockCanvas()
            if (c != null) {
                try { drawUi(c) } finally { holder.unlockCanvasAndPost(c) }
            }
        }
    }

    override fun surfaceCreated(h: SurfaceHolder) {
        running = true
        lastT = 0L; acc = 0L
        track = makeTrack()
        val ht = android.os.HandlerThread("render", Process.THREAD_PRIORITY_DISPLAY)
        ht.start()
        renderThread = ht
        android.os.Handler(ht.looper).post {
            android.view.Choreographer.getInstance().postFrameCallback(vsync)
        }
    }
    override fun surfaceChanged(h: SurfaceHolder, f: Int, w: Int, hh: Int) {}
    override fun surfaceDestroyed(h: SurfaceHolder) {
        running = false
        renderThread?.quitSafely(); renderThread?.join()
        track?.stop(); track?.release(); track = null
    }
}
