<script setup lang="ts">
import { onMounted, ref, useTemplateRef } from 'vue'

const drawerContents = useTemplateRef('drawer-content')

const open = ref(false)

let mouseon = false

function mousemove(event: MouseEvent) {}

function mousedown(event: MouseEvent) {
    mouseon = true
}

function mouseup(event: MouseEvent) {
    mouseon = false
}

const containerHeight = ref(0)

let resizeObserver = new ResizeObserver(entries => {
    for (const entry of entries) {
        const rect = entry.contentRect

        console.log(rect)

        containerHeight.value = rect.height
    }
})

function click() {
    open.value = !open.value

    console.log('click', open.value)
}

onMounted(() => {
    if (!drawerContents.value) return

    resizeObserver.observe(drawerContents.value)
})
</script>

<template>
    <div class="drawer">
        <div class="knob-grab" @mousedown="mousedown" @mouseup="mouseup" @mousemove="mousemove" @click="click">
            <div class="knob"></div>
        </div>

        <div class="container" :style="{ height: open ? `${containerHeight}px` : '0px' }">
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
