<script setup lang="ts">
import TopBar from '../components/TopBar.vue'
import LiveView from '../components/LiveView.vue'
</script>

<template>
    <TopBar />

    <LiveView />
</template>

<style>
body {
    margin: 0;
    background: #1d1d1d;
}

:root {
    font-family: Inter, Avenir, Helvetica, Arial, sans-serif;
    font-size: 16px;
    line-height: 24px;
    font-weight: 400;

    color: #ffffff;

    font-synthesis: none;
    text-rendering: optimizeLegibility;
    -webkit-font-smoothing: antialiased;
    -moz-osx-font-smoothing: grayscale;
    -webkit-text-size-adjust: 100%;
}
</style>
