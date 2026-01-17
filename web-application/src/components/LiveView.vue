<script setup lang="ts">
import { computed, onMounted, ref, useTemplateRef } from 'vue'
import { liveViewLoaded } from '../store/store'
import { isPermissionGranted, requestPermission, sendNotification } from '@tauri-apps/plugin-notification'

const ip = ref('10.142.233.231')
const streamUrl = computed(() => 'http://' + ip.value + ':81/stream')

const image = useTemplateRef('image')

async function onLiveViewLoaded() {
    liveViewLoaded.value = true

    let permissionGranted = await isPermissionGranted()

    if (!permissionGranted) {
        const permission = await requestPermission()
        permissionGranted = permission === 'granted'
    }

    if (permissionGranted) {
        sendNotification({ title: 'THING', body: 'Live view has loaded!' })
    }
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
})
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
