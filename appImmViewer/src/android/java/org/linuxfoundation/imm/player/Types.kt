package org.linuxfoundation.imm.player

/** * Async error handling */
sealed class Result<out T : Any>

data class Success<out T : Any>(val data: T) : Result<T>()

data class Failure(val message: Message) : Result<Nothing>()

data class Asset(
    val path: String,
    val timestamp: Long = System.currentTimeMillis(),
    val version: Int = 0
)

data class Quillustration(
    val imm: Asset,
    val version: Int = 0,
    val timestamp: Long = System.currentTimeMillis()
) {}

enum class Imm(val value: String) {
  Error("error"),
  Loading("http"),
}

enum class PaintRenderingTechnique(val value: Int) {
  Static(0),
  Pretessellated(1),
  StaticTexture(3)
}

// Corresponds to MessageType in OvrApp.cpp
enum class Message(val value: Int) {
  LoadImmPath(0),
  ErrorUnknown(1),
  Disconnected(2),
  UpdateViewpoints(5)
}
