<script setup lang="ts">
import { computed, ref } from 'vue'

const ip = ref('10.142.233.231')
const streamUrl = computed(() => 'http://' + ip.value + ':81/stream')
</script>

<template>
    <main>
        <section>
            <h1>Live View</h1>
            <div>
                <!-- placeholder div where video will go -->
            </div>
        </section>

        <!-- Bottom control buttons -->
        <section>
            <button>Speak</button>
            <button>Mute</button>
            <img :src="streamUrl" />
        </section>
    </main>
</template>
