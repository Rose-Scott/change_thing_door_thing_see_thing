<script setup lang="ts">
import ReplayCard from './Replay-Card.vue';

const clips = [
  {
    date: "2023-06-01",
    timestamp: "12:00:00",
    duration: 120,
    videoURL: "https://example.com/video1.mp4",
    thumbnailURL: "https://example.com/thumb1.jpg"
  },
  {
    date: "2023-06-02",
    timestamp: "14:30:00",
    duration: 90,
    videoURL: "https://example.com/video2.mp4",
    thumbnailURL: "https://example.com/thumb2.jpg"
  }
];
</script>

<template>
  <main>
    <h2>Replays</h2>

    <section>
      <ReplayCard
        v-for="clip in clips"
        :date="clip.date"
        :timestamp="clip.timestamp"
        :videoDuration="clip.duration"
        :videoURL="clip.videoURL"
        :thumbnailURL="clip.thumbnailURL"
      />
    </section>
  </main>
</template>

