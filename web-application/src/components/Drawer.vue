<script setup lang="ts">
import { onMounted, ref, useTemplateRef } from 'vue'

const open = ref(false)

const containerHeight = ref(0)

async function click() {
    open.value = !open.value

    if (open.value) {
        containerHeight.value = 350
    } else {
        containerHeight.value = 0
    }
}
</script>

<template>
    <div class="drawer" ref="drawer">
        <div class="knob-grab" @click="click">
            <div class="knob"></div>
        </div>

        <div class="container" :style="{ height: `${containerHeight}px` }">
            <div class="drawer-content">
                <div class="drawer-content-sizing">
                    <div class="events">
                        <p class="event-title">Events:</p>

                        <div class="event">
                            <p>Test</p>
                        </div>

                        <div class="event">
                            <p>Test</p>
                        </div>

                        <div class="event">
                            <p>Test</p>
                        </div>

                        <div class="event">
                            <p>Test</p>
                        </div>

                        <div class="event">
                            <p>Test</p>
                        </div>

                        <div class="event">
                            <p>Test</p>
                        </div>

                        <div class="event">
                            <p>Test</p>
                        </div>

                        <div class="event">
                            <p>Test</p>
                        </div>

                        <div class="event">
                            <p>Test</p>
                        </div>

                        <div class="event">
                            <p>Test</p>
                        </div>

                        <div class="event">
                            <p>Test</p>
                        </div>

                        <div class="event">
                            <p>Test</p>
                        </div>

                        <div class="event">
                            <p>Test</p>
                        </div>

                        <div class="spacing"></div>
                    </div>
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
    justify-content: stretch;
    align-items: stretch;

    flex-direction: column;

    border-top-left-radius: 1rem;
    border-top-right-radius: 1rem;

    background: #090909;

    align-self: stretch;
}

.container {
    overflow-y: hidden;

    transition: height 200ms ease-in-out;
}

.drawer-content {
    margin-left: 0.5rem;
    margin-right: 0.5rem;
    padding-left: 0.5rem;
    padding-right: 0.5rem;

    overflow-y: auto;
    scrollbar-color: #1d1d1d transparent;
    scrollbar-width: thin;

    height: 100%;
}

.drawer-content-sizing {
    display: flex;
}

.events {
    display: flex;
    flex-direction: column;

    width: 100%;
}

.event-title {
    font-weight: 500;
    font-size: 16px;

    padding-bottom: 1rem;

    border-bottom: solid 1px #1d1d1d;
}

.event {
    border-bottom: solid 1px #1d1d1d;
}

.event > p {
    margin-top: 0.5rem;
    margin-bottom: 0.5rem;
}

.spacing {
    height: 1rem;
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
