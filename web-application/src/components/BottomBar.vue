<script setup lang="ts"></script>

<template>
    <div class="bottom-bar">
        <button></button>
    </div>
</template>

<style scoped>
p {
    margin: 0;
}

.bottom-bar {
    display: flex;
    justify-content: center;
    align-items: center;

    background: #090909;

    min-height: 4rem;

    align-self: stretch;

    border-top: solid 1px #1d1d1d;
}

button {
    width: 2.5rem;
    height: 2.5rem;

    border-radius: 100%;

    outline: none;
    border: none;

    background-color: #fff7db;
}
</style>
