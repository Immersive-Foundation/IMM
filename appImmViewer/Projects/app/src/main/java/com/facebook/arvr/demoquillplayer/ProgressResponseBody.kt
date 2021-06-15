package com.facebook.arvr.demoquillplayer

import java.io.IOException
import okhttp3.MediaType
import okhttp3.ResponseBody
import okio.*

interface ProgressListener {
  fun listenFor(id: String)
  fun update(
      startTime: Long,
      progressId: String,
      bytesRead: Long,
      contentLength: Long,
      done: Boolean
  )
}

class ProgressResponseBody
internal constructor(
    private val tag: String,
    private val responseBody: ResponseBody?,
    private val progressListener: ProgressListener,
    private val startTime: Long = System.currentTimeMillis()
) : ResponseBody() {
  private var bufferedSource: BufferedSource? = null

  override fun contentType(): MediaType? {
    return responseBody!!.contentType()
  }

  override fun contentLength(): Long {
    return responseBody!!.contentLength()
  }

  override fun source(): BufferedSource {
    if (bufferedSource == null) {
      bufferedSource = Okio.buffer(source(responseBody!!.source()))
    }
    return bufferedSource!!
  }

  private fun source(source: Source): Source {
    return object : ForwardingSource(source) {
      var totalBytesRead = 0L

      @Throws(IOException::class)
      override fun read(sink: Buffer, byteCount: Long): Long {
        val bytesRead = super.read(sink, byteCount)
        totalBytesRead += if (bytesRead != -1L) bytesRead else 0
        progressListener.update(
            startTime, tag, totalBytesRead, responseBody!!.contentLength(), bytesRead == -1L)
        return bytesRead
      }
    }
  }
}
