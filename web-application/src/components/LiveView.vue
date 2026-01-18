<script setup lang="ts">
import { computed, onMounted, ref, useTemplateRef } from 'vue'
import { liveViewLoaded } from '../store/store'
import { isPermissionGranted, requestPermission, sendNotification } from '@tauri-apps/plugin-notification'

const ip = ref('10.142.233.231')
const streamUrl = computed(() => 'http://' + ip.value + ':81/stream')
const audioStreamUrl = computed(() => 'http://' + ip.value + ':82/audio')

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
})

function float32ToInt16(float32Array: Float32Array) {
    const int16Array = new Int16Array(float32Array.length)
    for (let i = 0; i < float32Array.length; i++) {
        const s = Math.max(-1, Math.min(1, float32Array[i]))
        int16Array[i] = s < 0 ? s * 0x8000 : s * 0x7fff
    }
    return int16Array
}

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
