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
        return m
    }

    override fun onTouchEvent(e: MotionEvent): Boolean {
        var m = 0
        val act = e.actionMasked
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
        c.drawBitmap(bmp, null, gameRect, null)
        val m = mask
        btn(c, dpx, dpy - dr * 0.65f, dr * 0.32f, "^", m and UP != 0)
        btn(c, dpx, dpy + dr * 0.65f, dr * 0.32f, "v", m and DOWN != 0)
        btn(c, dpx - dr * 0.65f, dpy, dr * 0.32f, "<", m and LEFT != 0)
        btn(c, dpx + dr * 0.65f, dpy, dr * 0.32f, ">", m and RIGHT != 0)
        btn(c, ax, ay, br, "A", m and A != 0)
        btn(c, bx, by, br, "B", m and B != 0)
        fill.color = Color.WHITE
        fill.alpha = if (m and SELECT != 0) 110 else 40
        c.drawRoundRect(selRect, 20f, 20f, fill); c.drawRoundRect(selRect, 20f, 20f, line)
        c.drawText("SELECT", selRect.centerX(), selRect.centerY() + txt.textSize / 3f, txt)
        fill.alpha = if (m and START != 0) 110 else 40
        c.drawRoundRect(startRect, 20f, 20f, fill); c.drawRoundRect(startRect, 20f, 20f, line)
        c.drawText("START", startRect.centerX(), startRect.centerY() + txt.textSize / 3f, txt)
    }

    // Satu thread: 1 frame game -> 800 sampel audio -> write blocking (audio = jam utama)
    private fun gameLoop() {
        Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO)
        val rate = 48000
        val chunk = 800
        val track = AudioTrack.Builder()
            .setAudioAttributes(AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION).build())
            .setAudioFormat(AudioFormat.Builder()
                .setEncoding(AudioFormat.ENCODING_PCM_FLOAT)
                .setSampleRate(rate)
                .setChannelMask(AudioFormat.CHANNEL_OUT_MONO).build())
            .setBufferSizeInBytes(chunk * 4 * 6)
            .setTransferMode(AudioTrack.MODE_STREAM)
            .build()
        val buf = FloatArray(chunk)
        val silence = FloatArray(chunk)
        track.write(silence, 0, chunk, AudioTrack.WRITE_NON_BLOCKING)
        track.write(silence, 0, chunk, AudioTrack.WRITE_NON_BLOCKING)
        track.play()
        while (running) {
            Core.frame(pixels)
            Core.audio(buf, chunk)
            track.write(buf, 0, chunk, AudioTrack.WRITE_BLOCKING)
            synchronized(bmp) { bmp.setPixels(pixels, 0, sw, 0, 0, sw, sh) }
            dirty = true
        }
        track.stop(); track.release()
    }

    private fun drawLoop() {
        Process.setThreadPriority(Process.THREAD_PRIORITY_DISPLAY)
        while (running) {
            if (!dirty) { Thread.sleep(2); continue }
            dirty = false
            val c = if (android.os.Build.VERSION.SDK_INT >= 26) holder.lockHardwareCanvas() else holder.lockCanvas()
            if (c != null) {
                try { synchronized(bmp) { drawUi(c) } } finally { holder.unlockCanvasAndPost(c) }
            }
        }
    }

    override fun surfaceCreated(h: SurfaceHolder) {
        running = true
        gameThread = Thread { gameLoop() }.also { it.start() }
        drawThread = Thread { drawLoop() }.also { it.start() }
    }
    override fun surfaceChanged(h: SurfaceHolder, f: Int, w: Int, hh: Int) {}
    override fun surfaceDestroyed(h: SurfaceHolder) {
        running = false
        gameThread?.join(); drawThread?.join()
    }
}
