<script setup lang="ts"></script>

<template>
    <div class="top-bar">
        <p>Live View</p>
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

    background-image: linear-gradient(#000000ac, #00000000);

    padding: 0.5rem;
    padding-bottom: 1rem;
    padding-top: 3rem;

    position: absolute;
    top: 0;
    left: 0;

    width: calc(100% - 1rem);
}
</style>
