<script setup lang="ts">
import { ref, computed, onMounted, onBeforeUnmount } from 'vue'

const ip = '172.27.187.231'
const streamUrl = computed(() => `http://${ip}:81/stream`)
const status = ref<HTMLElement | null>(null) // Fixed: proper ref type [web:23]

let audioCtx: AudioContext | null = null
let isPlaying = true // Fixed: start true for continuous play
let nextStartTime = 0

async function startAudioStream() {
  try {
    audioCtx = new window.AudioContext()
    const LATENCY_BUFFER = 0.1
    nextStartTime = audioCtx.currentTime + LATENCY_BUFFER

    const response = await fetch(`${window.location.protocol}//${window.location.hostname}:82/audio`)
    if (!response.ok) throw new Error(`HTTP ${response.status}`)

    const reader = response.body!.getReader()
    status.value!.textContent = 'Audio: Buffering...' // Safe after ref

    let chunks: Uint8Array[] = [] // Fixed: typed array
    let totalBytes = 0
    let initialBufferFilled = false
    const INITIAL_BUFFER = 6400
    const CHUNK_THRESHOLD = 3200

    while (isPlaying) {
      const { done, value } = await reader.read()
      if (done) {
        status.value!.textContent = 'Audio: Stream ended'
        break
      }

      if (!value || value.length === 0) continue

      chunks.push(value)
      totalBytes += value.length

      const threshold = initialBufferFilled ? CHUNK_THRESHOLD : INITIAL_BUFFER
      if (totalBytes >= threshold) {
        initialBufferFilled = true
        const combined = new Uint8Array(totalBytes)
        let offset = 0
        for (const chunk of chunks) {
          combined.set(chunk, offset)
          offset += chunk.length
        }
        chunks = []
        totalBytes = 0

        const len = combined.length - (combined.length % 2)
        if (len === 0) continue

        const dataView = new DataView(combined.buffer, combined.byteOffset, len)
        const float32 = new Float32Array(len / 2)
        for (let i = 0; i < float32.length; i++) {
          float32[i] = dataView.getInt16(i * 2, true) / 32768.0
        }

        const buffer = audioCtx!.createBuffer(1, float32.length, 16000)
        buffer.getChannelData(0).set(float32)

        const source = audioCtx!.createBufferSource()
        source.buffer = buffer

        const gainNode = audioCtx!.createGain()
        source.connect(gainNode)
        gainNode.connect(audioCtx!.destination)

        if (nextStartTime < audioCtx.currentTime) {
          nextStartTime = audioCtx.currentTime + 0.05
          gainNode.gain.setValueAtTime(0, nextStartTime)
          gainNode.gain.linearRampToValueAtTime(1, nextStartTime + 0.05)
        } else {
          gainNode.gain.setValueAtTime(1, nextStartTime)
        }
        source.start(nextStartTime)
        nextStartTime += buffer.duration

        status.value!.textContent = 'Audio: Playing'
      }
    }
  } catch (error) {
    console.error('Audio stream error:', error)
    if (status.value) status.value.textContent = `Audio: Error - ${error}` // Fixed typo
  }
}

onMounted(() => {
  startAudioStream()
})

onBeforeUnmount(() => {
  isPlaying = false
  audioCtx?.close()
})
async function unmuteAudio() {
  if (audioCtx?.state === 'suspended') {
    await audioCtx.resume()
    console.log('AudioContext resumed')
  }
  if (status.value) status.value.textContent = 'Audio: Clicked - check console'
}
</script>

<template>
  <main>
    <section>
      <h1>Live View</h1>
      <div>
        <img :src="streamUrl" alt="Live stream" />
      </div>
      <p ref="status">Audio: Initializing...</p> <!-- Fixed: add ref -->
    </section>
  </main>
</template>