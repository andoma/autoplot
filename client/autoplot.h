// This code is in the public domain

#pragma once

/**
 * API usage example:
 *
 * Multiple metrics can be updated in a single API call.
 *
 *
 *
 * Gauge - Displayed as-is in the UI
 *
 *   double t = 37;
 *   autoplot(AUTOPLOT_GAUGE, "temp", 37, NULL);
 *
 *
 * Counters - Displayed as derivate in the UI (ie packets turns into packets/s)
 *
 *   struct {
 *      uint64_t packets, bytes;
 *   } counters;
 *
 *
 *   For every packet received:
 *
 *   counters.packets++;
 *   counters.bytes += size;
 *
 *   autoplot(AUTOPLOT_COUNTER, "pps", counters.packets,
 *            AUTOPLOT_COUNTER, "bps", counters.bytes * 8, // convert to bits
 *            NULL);
 *
 */

typedef enum {
  AUTOPLOT_END = 0,      // Argument sentinel, not a real kind
  AUTOPLOT_GAUGE = 1,    // double
  AUTOPLOT_COUNTER = 2,  // uint64_t
} autoplot_kind_t;

void autoplot(autoplot_kind_t kind, ...) __attribute__((__sentinel__(0)));
