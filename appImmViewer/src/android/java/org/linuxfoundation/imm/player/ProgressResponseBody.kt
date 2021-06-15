package org.linuxfoundation.imm.player

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
