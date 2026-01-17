<script setup lang="ts">
import { onMounted, ref, useTemplateRef } from 'vue'

const drawerContents = useTemplateRef('drawer-content')

const open = ref(false)

const containerFullHeight = ref(0)
const containerHeight = ref(0)

let resizeObserver = new ResizeObserver(entries => {
    for (const entry of entries) {
        const rect = entry.contentRect

        containerFullHeight.value = rect.height
    }
})

function click() {
    open.value = !open.value

    if (open.value) {
        containerHeight.value = containerFullHeight.value
    } else {
        containerHeight.value = 0
    }
}

onMounted(() => {
    if (!drawerContents.value) return

    resizeObserver.observe(drawerContents.value)
})
</script>

<template>
    <div class="drawer" ref="drawer">
        <div class="knob-grab" @click="click">
            <div class="knob"></div>
        </div>

        <div class="container" :style="{ height: `${containerHeight}px` }">
            <div class="drawer-content" ref="drawer-content">
                <div class="events-container">
                    <p>Test</p>
                    <p>Test</p>
                    <p>Test</p>
                    <p>Test</p>
                    <p>Test</p>
                    <p>Test</p>
                    <p>Test</p>
                    <p>Test</p>
                    <p>Test</p>
                    <p>Test</p>
                    <p>Test</p>
                    <p>Test</p>
                    <p>Test</p>
                    <p>Test</p>
                    <p>Test</p>
                    <p>Test</p>
                    <p>Test</p>
                    <p>Test</p>
                    <p>Test</p>
                </div>
            </div>
        </div>
    </div>
</template>

<style scoped>
p {
    margin: 0;
}

.drawer {
    display: flex;
    align-items: center;

    flex-direction: column;

    border-top-left-radius: 1rem;
    border-top-right-radius: 1rem;

    background: #090909;

    align-self: stretch;
}

.container {
    overflow-y: hidden;

    transition: height 200ms ease;
}

.drawer-contenr {
    display: flex;
}

.events-container {
    padding: 1rem;
}

.knob-grab {
    align-self: stretch;

    min-height: 1rem;

    display: flex;
    justify-content: center;
    align-items: center;
}

.knob {
    width: 2.5rem;
    height: 0.3rem;

    background-color: #1d1d1d;

    border-radius: 0.3rem;
}
</style>
