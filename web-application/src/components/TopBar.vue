<script setup lang="ts">
import { liveViewLoaded } from '../store/store'
</script>

<template>
    <div class="top-bar">
        <div class="status-holder">
            <p>Live View</p>
            <div class="dot" :class="{ loaded: liveViewLoaded }"></div>
        </div>
        <p>22%</p>
    </div>
</template>

<style scoped>
p {
    margin: 0;
}

.top-bar {
    display: flex;
    justify-content: space-between;

    background-image: linear-gradient(#000000bc, #00000000);

    padding: 0.5rem;
    padding-bottom: 2rem;
    padding-top: 3rem;

    position: absolute;
    top: 0;
    left: 0;

    width: calc(100% - 1rem);
}

.status-holder {
    display: flex;

    align-items: center;
    gap: 0.5rem;
}

.dot {
    width: 0.75rem;
    height: 0.75rem;

    background-color: #ffea00;

    border-radius: 100%;
}

.dot.loaded {
    background-color: hsl(136, 100%, 50%);
}
</style>
