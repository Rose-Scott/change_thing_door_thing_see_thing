<script setup lang="ts"></script>

<template>
    <div class="drawer">
        <div class="knob"></div>

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

.events-container {
    padding: 1rem;
}

.knob {
    width: 2.5rem;
    height: 0.3rem;

    background-color: #1d1d1d;

    border-radius: 0.3rem;

    margin-top: 0.5rem;
}
</style>
