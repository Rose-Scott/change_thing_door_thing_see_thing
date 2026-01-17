<script setup lang="ts">
const props = defineProps<{
  date: string
  timestamp: string
  videoDuration: number
  videoURL: string
  thumbnailURL: string
}>()
</script>

<template>
  <div>
    <img :src="props.thumbnailURL" alt="Replay thumbnail" />
    <p>{{ props.date }} {{ props.timestamp }}</p>
    <p>time: {{ props.timestamp }} seconds</p>
  </div>
</template>