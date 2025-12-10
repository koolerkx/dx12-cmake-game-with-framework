#pragma once

#include <cstdint>

// RenderLayer: Defines which rendering categories an object belongs to
// Uses bitflags for multi-layer support
enum class RenderLayer : uint8_t {
  None = 0,
  Opaque = 1 << 0,       // Opaque objects
  Transparent = 1 << 1,  // Transparent objects
  Shadow = 1 << 2,       // Shadow casting objects
  UI = 1 << 3,           // UI elements
  Particle = 1 << 4,     // Particle effects
  Debug = 1 << 5,        // Debug visualization
  All = 0xFF             // All layers
};

// Bitwise operators for RenderLayer
inline RenderLayer operator|(RenderLayer a, RenderLayer b) {
  return static_cast<RenderLayer>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline RenderLayer operator&(RenderLayer a, RenderLayer b) {
  return static_cast<RenderLayer>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline RenderLayer operator~(RenderLayer a) {
  return static_cast<RenderLayer>(~static_cast<uint8_t>(a));
}

inline RenderLayer& operator|=(RenderLayer& a, RenderLayer b) {
  a = a | b;
  return a;
}

inline RenderLayer& operator&=(RenderLayer& a, RenderLayer b) {
  a = a & b;
  return a;
}

// RenderTag: Fine-grained classification for render objects
// Uses bitflags for multi-tag support
enum class RenderTag : uint8_t {
  None = 0,
  Static = 1 << 0,      // Static objects (no movement)
  Dynamic = 1 << 1,     // Dynamic objects (can move)
  Lit = 1 << 2,         // Affected by lighting
  Unlit = 1 << 3,       // Not affected by lighting
  CastShadow = 1 << 4,  // Casts shadows
  All = 0xFF            // All tags
};

// Bitwise operators for RenderTag
inline RenderTag operator|(RenderTag a, RenderTag b) {
  return static_cast<RenderTag>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline RenderTag operator&(RenderTag a, RenderTag b) {
  return static_cast<RenderTag>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline RenderTag operator~(RenderTag a) {
  return static_cast<RenderTag>(~static_cast<uint8_t>(a));
}

inline RenderTag& operator|=(RenderTag& a, RenderTag b) {
  a = a | b;
  return a;
}

inline RenderTag& operator&=(RenderTag& a, RenderTag b) {
  a = a & b;
  return a;
}

// Helper functions
inline bool HasLayer(RenderLayer value, RenderLayer test) {
  return (static_cast<uint8_t>(value) & static_cast<uint8_t>(test)) != 0;
}

inline bool HasTag(RenderTag value, RenderTag test) {
  return (static_cast<uint8_t>(value) & static_cast<uint8_t>(test)) != 0;
}

inline bool HasAnyTag(RenderTag value, RenderTag mask) {
  return (static_cast<uint8_t>(value) & static_cast<uint8_t>(mask)) != 0;
}

inline bool HasAllTags(RenderTag value, RenderTag mask) {
  return (static_cast<uint8_t>(value) & static_cast<uint8_t>(mask)) == static_cast<uint8_t>(mask);
}
