<script setup lang="ts">
import { computed, onMounted, ref, useTemplateRef } from 'vue'
import { liveViewLoaded } from '../store/store'
import { isPermissionGranted, requestPermission, sendNotification } from '@tauri-apps/plugin-notification'

const ip = ref('172.27.187.231')
const streamUrl = computed(() => 'http://' + ip.value + ':81/stream')

const image = useTemplateRef('image')

// async function onLiveViewLoaded() {
//     liveViewLoaded.value = true

//     let permissionGranted = await isPermissionGranted()

//     if (!permissionGranted) {
//         const permission = await requestPermission()
//         permissionGranted = permission === 'granted'
//     }

//     if (permissionGranted) {
//         sendNotification({ title: 'THING', body: 'Live view has loaded!' })
//     }
// }

let audioCtx: AudioContext | null = null
let isPlaying = true // Fixed: start true for continuous play
let nextStartTime = 0

async function startAudioStream() {
  try {
    audioCtx = new window.AudioContext()
    const LATENCY_BUFFER = 0.5
    nextStartTime = audioCtx.currentTime + LATENCY_BUFFER

    const response = await fetch('http://' + ip.value + ':82/audio')
    if (!response.ok) throw new Error(`HTTP ${response.status}`)

    const reader = response.body!.getReader()
    console.log('Audio: Buffering...') 

    let chunks: Uint8Array[] = [] // Fixed: typed array
    let totalBytes = 0
    let initialBufferFilled = false
    const INITIAL_BUFFER = 6400
    const CHUNK_THRESHOLD = 3200

    while (isPlaying) {
      const { done, value } = await reader.read()
      if (done) {
        console.log('Audio: Stream ended')
        window.location.reload()
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

        console.log('Audio: Playing')
      }
    }
  } catch (error) {
    console.error('Audio stream error:', error)
    console.log(`Audio: Error - ${error}`)
    window.location.reload()
  }
}

async function onErrorLoadingLiveView() {
    ip.value = ''

    setTimeout(() => {
        ip.value = '172.27.187.231'
    }, 100)
}

onMounted(() => {
    if (!image.value) return

    image.value.onload = ()=>{liveViewLoaded.value = true; startAudioStream();}//onLiveViewLoaded
    image.value.onerror = onErrorLoadingLiveView
})
</script>

<template>
    <div class="center-view">

        <img v-show="liveViewLoaded" :src="streamUrl" ref="image" />
    </div>
</template>

<style scoped>
img {
    background: white;
    width: 100vw;
}

.center-view {
    display: flex;

    justify-content: center;
    align-items: center;

    align-self: stretch;

    flex-grow: 1;
    min-height: 0;
}

.live-view-preview {
    width: 100vw;
    background: white;
    aspect-ratio: 1;
}
</style>
