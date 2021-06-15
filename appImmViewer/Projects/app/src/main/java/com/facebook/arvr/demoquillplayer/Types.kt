package com.facebook.arvr.demoquillplayer

import kotlinx.serialization.*
import kotlinx.serialization.SerializationException
import kotlinx.serialization.json.Json

/** * Async error handling */
sealed class Result<out T : Any>

data class Success<out T : Any>(val data: T) : Result<T>()

data class Failure(val message: Message) : Result<Nothing>()

@Serializable
data class EntQuillustration(
    val id: String,
    val immUrl: String,
    val artistImageUrl: String?,
    val previewImageUrl: String?,
    val title: String?,
    val artistName: String?,
    val description: String?,
    val creationTime: Int,
    var lowLatencyAudio: Boolean,
    var artistImage: Asset? = null,
    var previewImage: Asset? = null
) {
  private var cursor: String? = null

  fun setCursor(c: String?): EntQuillustration {
    cursor = c
    return this
  }

  fun getCursor(): String? {
    return cursor
  }
}

@Serializable
data class Asset(
    val path: String,
    val timestamp: Long = System.currentTimeMillis(),
    val version: Int = 0
)

@Serializable
data class Quillustration(
    val ent: EntQuillustration,
    val imm: Asset,
    val version: Int = 0,
    val timestamp: Long = System.currentTimeMillis()
) {
  companion object {
    // An hour
    val ttl: Long = 60 * 60 * 1000

    fun fromString(s: String): Quillustration? {
      return try {
        Json.decodeFromString<Quillustration>(s)
      } catch (e: SerializationException) {
        null
      }
    }
  }

  val isFresh: Boolean
    get() = ttl > System.currentTimeMillis() - timestamp

  override fun toString(): String {
    return Json.encodeToString(this)
  }
}

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
