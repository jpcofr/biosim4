/**
 * @file feedForward.cpp
 * @brief Evaluate an individual's neural network to produce action levels.
 *
 * The feed-forward step is the only time an individual's neural network is
 * executed during a simulation tick. Sensors drive inputs, internal neurons
 * integrate signals, and the resulting action activations are returned to be
 * interpreted by `executeActions.cpp`.
 */

#include "simulator.h"

#include <cassert>
#include <cmath>
#include <iostream>

namespace BioSim {

/**
 * @brief Run a full neural feed-forward pass for one individual.
 * @param simStep Simulation step counter passed to time-sensitive sensors.
 * @return Array of raw action activations (prior to probability scaling).
 *
 * @details
 * **Operational phases**
 * 1. **Sensor sampling** – every connection with `SENSOR` source reads from
 *    `getSensor()` which already clamps to `[SENSOR_MIN, SENSOR_MAX]`.
 * 2. **Hidden integration** – weighted inputs accumulate per internal neuron in
 *    `neuronAccumulators`. When the first `ACTION` sink is encountered (the
 *    connections are pre-sorted), all driven neurons are squashed through
 *    `tanh()` so their outputs stay within `[NEURON_MIN, NEURON_MAX]`.
 * 3. **Action accumulation** – remaining connections target action sinks;
 *    their weighted sums form the returned `actionLevels` array. These values
 *    intentionally retain arbitrary ranges because `executeActions()` maps
 *    them to probabilities according to the action semantics.
 *
 * There is no learning during the agent's lifetime; weights and topology are
 * fixed when the genome is decoded in `createWiringFromGenome()`. Persistent
 * neuron outputs allow for recurrent behavior without explicit feedback edges.
 */
std::array<float, Action::NUM_ACTIONS> Individual::feedForward(unsigned simStep) {
  /// This container is used to return values for all the action outputs. This array
  /// contains one value per action neuron, which is the sum of all its weighted
  /// input connections. The sum has an arbitrary range. Return by value assumes compiler
  /// return value optimization.
  std::array<float, Action::NUM_ACTIONS> actionLevels;
  actionLevels.fill(0.0);  ///< undriven actions default to value 0.0

  /// Weighted inputs to each neuron are summed in neuronAccumulators[]
  std::vector<float> neuronAccumulators(nnet.neurons.size(), 0.0);

  /// Connections were ordered at birth so that all connections to neurons get
  /// processed here before any connections to actions. As soon as we encounter the
  /// first connection to an action, we'll pass all the neuron input accumulators
  /// through a transfer function and update the neuron outputs in the indiv,
  /// except for undriven neurons which act as bias feeds and don't change. The
  /// transfer function will leave each neuron's output in the range -1.0..1.0.

  bool neuronOutputsComputed = false;
  for (Gene& conn : nnet.connections) {
    if (conn.sinkType == ACTION && !neuronOutputsComputed) {
      /// We've handled all the connections from sensors and now we are about to
      /// start on the connections to the action outputs, so now it's time to
      /// update and latch all the neuron outputs to their proper range (-1.0..1.0)
      for (unsigned neuronIndex = 0; neuronIndex < nnet.neurons.size(); ++neuronIndex) {
        if (nnet.neurons[neuronIndex].driven) {
          nnet.neurons[neuronIndex].output = std::tanh(neuronAccumulators[neuronIndex]);
        }
      }
      neuronOutputsComputed = true;
    }

    /// Obtain the connection's input value from a sensor neuron or other neuron
    /// The values are summed for now, later passed through a transfer function
    float inputVal;
    if (conn.sourceType == SENSOR) {
      inputVal = getSensor((Sensor)conn.sourceNum, simStep);
    } else {
      inputVal = nnet.neurons[conn.sourceNum].output;
    }

    /// Weight the connection's value and add to neuron accumulator or action accumulator.
    /// The action and neuron accumulators will therefore contain +- float values in
    /// an arbitrary range.
    if (conn.sinkType == ACTION) {
      actionLevels[conn.sinkNum] += inputVal * conn.weightAsFloat();
    } else {
      neuronAccumulators[conn.sinkNum] += inputVal * conn.weightAsFloat();
    }
  }

  return actionLevels;
}

}  // namespace BioSim
