<script setup lang="ts">
import { computed, ref, onMounted, onBeforeUnmount } from 'vue'

const ip = ref('10.142.233.231')
const streamUrl = computed(() => 'http://' + ip.value + ':81/stream')
const audioChunkUrl = computed(() => 'http://' + ip.value + ':81/audio-chunk')

//dumb audio stuff
let running = true
let audioEl: HTMLAudioElement | null = null

async function fetchAndPlayChunk() {
  if (!audioEl) {
    audioEl = document.getElementById('remote-audio') as HTMLAudioElement
  }

  const res = await fetch(audioChunkUrl.value)
  const data = await res.arrayBuffer()
  const blob = new Blob([data], { type: 'audio/wav' })
  const url = URL.createObjectURL(blob)

  audioEl.src = url
  await audioEl.play()
}

async function startReceiving() {
  if (running) return
  running = true

  while (running) {
    try {
      await fetchAndPlayChunk()
    } catch (e) {
      console.error('audio error', e)
      running = false
    }
  }
}

function stopReceiving() {
  running = false
  if (audioEl) {
    audioEl.pause()
  }
}

async function audioLoop() {
  while (running) {
    try {
      await fetchAndPlayChunk()
    } catch (e) {
      console.error('audio error', e)
      // small delay before retry to avoid tight error loop
      await new Promise(r => setTimeout(r, 200))
    }
  }
}

onMounted(() => {
  running = true
  audioLoop()
})

onBeforeUnmount(() => {
  running = false
  if (audioEl) {
    audioEl.pause()
  }
})
</script>

<template>
    <main>
        <section>
            <h1>Live View</h1>
            <div>
                <img :src="streamUrl" />
                <audio id="remote-audio" style="display: none" ></audio>
            </div>
        </section>

        <!-- Bottom control buttons -->
        <section>
            <button>Speak</button>
            <button>Mute</button>
        </section>
    </main>
</template>
