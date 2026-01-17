<script setup lang="ts">
import { computed, onMounted, ref, useTemplateRef } from 'vue'
import { liveViewLoaded } from '../store/store'

const ip = ref('10.142.233.231')
const streamUrl = computed(() => 'http://' + ip.value + ':81/stream')

const image = useTemplateRef('image')

onMounted(() => {
    if (!image.value) return

    image.value.onload = () => (liveViewLoaded.value = true)
})
</script>

<template>
    <div class="center-view">
        <img :src="streamUrl" ref="image" />
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
</style>
