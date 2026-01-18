<script setup lang="ts">
import { computed, onMounted, ref, useTemplateRef } from 'vue'
import { liveViewLoaded } from '../store/store'
import { isPermissionGranted, requestPermission, sendNotification } from '@tauri-apps/plugin-notification'

const ip = ref('10.142.233.231')
const streamUrl = computed(() => 'http://' + ip.value + ':81/stream')
const audioStreamUrl = computed(() => 'http://' + ip.value + ':82/audio')
const recordStreamUrl = computed(() => 'http://' + ip.value + ':82/record')

const image = useTemplateRef('image')

async function onLiveViewLoaded() {
    liveViewLoaded.value = true

    // let permissionGranted = await isPermissionGranted()

    // if (!permissionGranted) {
    //     const permission = await requestPermission()
    //     permissionGranted = permission === 'granted'
    // }

    // if (permissionGranted) {
    //     sendNotification({ title: 'THING', body: 'Live view has loaded!' })
    // }
}

async function onErrorLoadingLiveView() {
    ip.value = ''

    setTimeout(() => {
        ip.value = '10.142.233.231'
    }, 100)
}

onMounted(() => {
    if (!image.value) return

    image.value.onload = onLiveViewLoaded
    image.value.onerror = onErrorLoadingLiveView

    // dealWithAudio()

    setInterval(() => {
        recordAndPlayback()
    }, 5000)
})

async function recordAndPlayback() {
    console.log('Requesting recording from ESP32...')

    const response = await fetch(recordStreamUrl.value)
    const audioData = await response.arrayBuffer()

    console.log('Received audio:', audioData.byteLength, 'bytes')

    // Check the data
    const int16Array = new Int16Array(audioData)
    console.log('First 10 samples:', int16Array.slice(0, 10))
    console.log('Min:', Math.min(...int16Array))
    console.log('Max:', Math.max(...int16Array))

    // If all zeros or all same value, microphone isn't working
    if (Math.min(...int16Array) === Math.max(...int16Array)) {
        console.error('No audio data - microphone not working!')
        return
    }

    // Convert Int16 to Float32
    const float32Array = new Float32Array(int16Array.length)
    for (let i = 0; i < int16Array.length; i++) {
        float32Array[i] = int16Array[i] / 32768.0
    }

    console.log('Float32 range:', Math.min(...float32Array), 'to', Math.max(...float32Array))

    // Play the audio
    const audioContext = new AudioContext()
    console.log('AudioContext sample rate:', audioContext.sampleRate)

    // Create buffer with correct sample rate
    const audioBuffer = audioContext.createBuffer(1, float32Array.length, 16000)
    audioBuffer.copyToChannel(float32Array, 0)

    const source = audioContext.createBufferSource()
    source.buffer = audioBuffer
    source.connect(audioContext.destination)

    console.log('Starting playback...')
    source.start(0)

    source.onended = () => {
        console.log('Playback finished')
    }
}

// function float32ToInt16(float32Array: Float32Array) {
//     const int16Array = new Int16Array(float32Array.length)
//     for (let i = 0; i < float32Array.length; i++) {
//         const s = Math.max(-1, Math.min(1, float32Array[i]))
//         int16Array[i] = s < 0 ? s * 0x8000 : s * 0x7fff
//     }
//     return int16Array
// }

// let timer = 0

// async function dealWithAudio() {
//     const stream = await navigator.mediaDevices.getUserMedia({ audio: true })

//     const audioContext = new AudioContext()
//     const source = audioContext.createMediaStreamSource(stream)

//     const bufferSize = 4096
//     const processor = audioContext.createScriptProcessor(bufferSize, 1, 1)

//     processor.onaudioprocess = event => {
//         const pcmData = float32ToInt16(event.inputBuffer.getChannelData(0))

//         const blob = new Blob([pcmData], { type: 'application/octet-stream' })

//         if (liveViewLoaded && timer <= 0) {
//             console.log(pcmData)

//             sendAudio(blob)

//             timer = 5
//         }

//         timer--
//     }

//     source.connect(processor)
//     processor.connect(audioContext.destination)
// }

// async function sendAudio(data: Blob) {
//     await fetch(audioStreamUrl.value, {
//         method: 'POST',
//         headers: {
//             'Content-Type': 'application/octet-stream',
//         },
//         body: data,
//     })
// }
</script>

<template>
    <div class="center-view">
        <img v-show="liveViewLoaded" :src="streamUrl" ref="image" />
        <div v-show="!liveViewLoaded" class="live-view-preview"></div>
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
