<script setup lang="ts">
import TopBar from '../components/TopBar.vue'
import LiveView from '../components/LiveView.vue'
import BottomBar from '../components/BottomBar.vue'
import Drawer from '../components/Drawer.vue'
</script>

<template>
    <TopBar />

    <main>
        <LiveView />

        <Drawer />

        <BottomBar />
    </main>
</template>

<style scoped>
main {
    display: flex;

    flex-direction: column;

    width: 100vw;
    height: 100vh;
}
</style>

<style>
body {
    margin: 0;
    background: #1d1d1d;
}

:root {
    font-family: Inter, Avenir, Helvetica, Arial, sans-serif;
    font-size: 14px;
    line-height: 24px;
    font-weight: 300;

    color: #ffffff;

    font-synthesis: none;
    text-rendering: optimizeLegibility;
    -webkit-font-smoothing: antialiased;
    -moz-osx-font-smoothing: grayscale;
    -webkit-text-size-adjust: 100%;
}

* {
    overscroll-behavior: none;
}
</style>
