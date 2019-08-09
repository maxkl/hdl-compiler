
use std::borrow::Borrow;
use std::rc::{Rc, Weak};
use std::fs::File;
use std::io::BufWriter;

use serde::{Serialize, Deserialize};
use serde_json;
use matches::matches;

use crate::shared::intermediate::{IntermediateBlock, IntermediateOp, Intermediate};
use crate::backend;
use crate::backend::BackendError;

struct Options {
    io_components: bool,
}

#[derive(Debug, Serialize, Deserialize)]
struct ConnectionData {
    x1: i32,
    y1: i32,
    x2: i32,
    y2: i32,
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(tag = "type")]
enum ComponentData {
    #[serde(rename = "const")]
    Const { value: bool },
    #[serde(rename = "and")]
    AND { inputs: u32 },
    #[serde(rename = "or")]
    OR { inputs: u32 },
    #[serde(rename = "xor")]
    XOR {},
    #[serde(rename = "not")]
    NOT {},
    #[serde(rename = "togglebutton")]
    ToggleButton {},
    #[serde(rename = "led")]
    LED { #[serde(rename = "offColor")] off_color: String, #[serde(rename = "onColor")] on_color: String },
    #[serde(rename = "custom")]
    Custom { #[serde(rename = "circuitName")] name: String },
    #[serde(rename = "input")]
    Input { label: String },
    #[serde(rename = "output")]
    Output { label: String },
}

#[derive(Debug, Serialize, Deserialize)]
struct CommonComponentData {
    x: i32,
    y: i32,

    #[serde(flatten)]
    data: ComponentData,
}

#[derive(Debug, Serialize, Deserialize)]
struct CircuitData {
    name: String,
    label: String,
    components: Vec<CommonComponentData>,
    connections: Vec<ConnectionData>,
}

#[derive(Debug, Serialize, Deserialize)]
struct FileData {
    version: u32,
    circuits: Vec<CircuitData>,
}

enum ComponentType {
    Connect,
    Const0,
    Const1,
    AND,
    OR,
    XOR,
    NOT,

    Circuit(String),
}

struct Component {
    typ: ComponentType,
    inputs: Vec<u32>,
    outputs: Vec<u32>,
}

struct Circuit {
    input_signal_count: u32,
    output_signal_count: u32,
    signal_count: u32,
    components: Vec<Component>,
}

impl Circuit {
    fn allocate_signals(&mut self, count: u32) -> u32 {
        let base_signal = self.signal_count;
        self.signal_count += count;
        base_signal
    }
}

pub struct LogicSimulator {
    //
}

impl LogicSimulator {
    pub fn run(output_path: Option<&str>, blocks: &Intermediate, args: &[&str]) -> backend::Result {
        let mut options = Options {
            io_components: true,
        };

        for &arg in args {
            match arg {
                "--no-io-components" => options.io_components = false,
                arg => return Err(BackendError::Custom(format!("unrecognized backend option '{}'", arg))),
            }
        }

        let output_path = output_path.unwrap_or("circuit.json");

        let circuits = Self::generate_circuits(blocks, &options)?;

        let data = FileData {
            version: 1,
            circuits,
        };

        let f = File::create(output_path)
            .map_err(|e| BackendError::Custom(format!("Unable to create output file: {}", e)))?;
        let writer = BufWriter::new(f);

        serde_json::to_writer(writer, &data)
            .map_err(|e| BackendError::Custom(format!("Unable to write JSON: {}", e)))?;

        Ok(())
    }

    fn generate_circuits(blocks: &Vec<Rc<IntermediateBlock>>, options: &Options) -> Result<Vec<CircuitData>, BackendError> {
        let main_found = blocks.iter()
            .any(|block| {
                let block: &IntermediateBlock = block.borrow();
                block.name == "main"
            });

        if !main_found {
            return Err(BackendError::Custom("main block not defined".to_string()));
        }

        let mut circuits = Vec::new();

        for block in blocks {
            let block = block.borrow();
            circuits.push(Self::generate_circuit(block, options, block.name == "main")?);
        }

        Ok(circuits)
    }

    fn generate_circuit(block: &IntermediateBlock, options: &Options, is_main: bool) -> Result<CircuitData, BackendError> {
        let io_signal_count = block.input_signal_count + block.output_signal_count;

        let mut circuit = Circuit {
            input_signal_count: block.input_signal_count,
            output_signal_count: block.output_signal_count,
            signal_count: io_signal_count,
            components: Vec::new(),
        };

        let mut nested_blocks_base_signals = Vec::new();

        for nested_block_weak in &block.blocks {
            let nested_block = Weak::upgrade(nested_block_weak).unwrap();

            nested_blocks_base_signals.push(circuit.signal_count);

            circuit.allocate_signals(nested_block.input_signal_count + nested_block.output_signal_count);
        }

        for (i, nested_block_weak) in block.blocks.iter().enumerate() {
            let nested_block = Weak::upgrade(nested_block_weak).unwrap();

            let nested_blocks_base_signal = nested_blocks_base_signals[i];

            let mut component = Component {
                typ: ComponentType::Circuit(nested_block.name.clone()),
                inputs: Vec::new(),
                outputs: Vec::new(),
            };

            for j in 0..nested_block.input_signal_count {
                component.inputs.push(nested_blocks_base_signal + j);
            }

            for j in 0..nested_block.output_signal_count {
                component.outputs.push(nested_blocks_base_signal + nested_block.input_signal_count + j);
            }

            circuit.components.push(component);
        }

        let mut max_signal_id = circuit.signal_count;

        for stmt in &block.statements {
            for &input_signal_id in &stmt.input_signal_ids {
                if input_signal_id >= max_signal_id {
                    max_signal_id = input_signal_id + 1;
                }
            }

            for &output_signal_id in &stmt.output_signal_ids {
                if output_signal_id >= max_signal_id {
                    max_signal_id = output_signal_id + 1;
                }
            }
        }

        circuit.allocate_signals(max_signal_id - circuit.signal_count);

        for stmt in &block.statements {
            let typ = match stmt.op {
                IntermediateOp::Connect => ComponentType::Connect,
                IntermediateOp::Const0 => ComponentType::Const0,
                IntermediateOp::Const1 => ComponentType::Const1,
                IntermediateOp::AND => ComponentType::AND,
                IntermediateOp::OR => ComponentType::OR,
                IntermediateOp::XOR => ComponentType::XOR,
                IntermediateOp::NOT => ComponentType::NOT,
                _ => return Err(BackendError::Custom(format!("unsupported op {:?} in intermediate statement", stmt.op))),
            };

            let mut component = Component {
                typ,
                inputs: Vec::new(),
                outputs: Vec::new(),
            };

            for &input_signal_id in &stmt.input_signal_ids {
                component.inputs.push(input_signal_id);
            }

            for &output_signal_id in &stmt.output_signal_ids {
                component.outputs.push(output_signal_id);
            }

            circuit.components.push(component);
        }

        Self::output_circuit(&circuit, &block.name, options, is_main)
    }

    fn output_circuit(circuit: &Circuit, name: &str, options: &Options, is_main: bool) -> Result<CircuitData, BackendError> {
        let mut circuit_data = CircuitData {
            name: name.to_string(),
            label: name.to_string(),
            components: Vec::new(),
            connections: Vec::new(),
        };

        let component_offset = (circuit.signal_count as i32 - 1) * 2 + 2;
        let output_connection_offset = component_offset + 13 + 2;

        let mut top_offset = 0;

        for component in &circuit.components {
            top_offset += 1;

            match &component.typ {
                ComponentType::Connect => {
                    circuit_data.connections.push(ConnectionData {
                        x1: component.inputs[0] as i32 * 2,
                        y1: top_offset,
                        x2: output_connection_offset + component.outputs[0] as i32 * 2,
                        y2: top_offset,
                    });
                },
                ComponentType::Const0 | ComponentType::Const1 => {
                    circuit_data.components.push(CommonComponentData {
                        x: (component_offset + 4) as i32,
                        y: top_offset,
                        data: ComponentData::Const {
                            value: matches!(component.typ, ComponentType::Const1)
                        },
                    });

                    top_offset += 4;
                },
                ComponentType::AND => {
                    circuit_data.components.push(CommonComponentData {
                        x: component_offset + 4,
                        y: top_offset,
                        data: ComponentData::AND {
                            inputs: component.inputs.len() as u32
                        },
                    });

                    top_offset += 1 + (component.inputs.len() as i32 - 1) * 2 + 1;
                },
                ComponentType::OR => {
                    circuit_data.components.push(CommonComponentData {
                        x: component_offset + 4,
                        y: top_offset,
                        data: ComponentData::OR {
                            inputs: component.inputs.len() as u32
                        },
                    });

                    top_offset += 1 + (component.inputs.len() as i32 - 1) * 2 + 1;
                },
                ComponentType::XOR => {
                    circuit_data.components.push(CommonComponentData {
                        x: component_offset + 4,
                        y: top_offset,
                        data: ComponentData::XOR {},
                    });

                    top_offset += 4;
                },
                ComponentType::NOT => {
                    circuit_data.components.push(CommonComponentData {
                        x: component_offset + 4,
                        y: top_offset,
                        data: ComponentData::NOT {},
                    });

                    top_offset += 4;
                },
                ComponentType::Circuit(name) => {
                    circuit_data.components.push(CommonComponentData {
                        x: component_offset + 1,
                        y: top_offset,
                        data: ComponentData::Custom {
                            name: name.clone(),
                        },
                    });

                    let max_pins = component.inputs.len().max(component.outputs.len()) as i32;

                    top_offset += 1 + (max_pins - 1) * 2 + 1;
                },
            }

            top_offset += 1;

            if !matches!(component.typ, ComponentType::Connect) {
                let max_pins = component.inputs.len().max(component.outputs.len()) as i32;
                let height = (4).max(1 + (max_pins - 1) * 2 + 1);
                let mid = height / 2;

                let component_top_offset = top_offset - 1 - height;
                let input_top_offset = component_top_offset + 1 + mid - component.inputs.len() as i32;
                let output_top_offset = component_top_offset + 1 + mid - component.outputs.len() as i32;

                let component_width = match component.typ {
                    ComponentType::Circuit(_) => 13,
                    _ => 7,
                };

                for (i, &input_signal_id) in component.inputs.iter().enumerate() {
                    let y = input_top_offset + 2 * i as i32;

                    circuit_data.connections.push(ConnectionData {
                        x1: input_signal_id as i32 * 2,
                        y1: y,
                        x2: component_offset + 13 / 2 - component_width / 2,
                        y2: y,
                    });
                }

                for (i, &output_signal_id) in component.outputs.iter().enumerate() {
                    let y = output_top_offset + i as i32 * 2;

                    circuit_data.connections.push(ConnectionData {
                        x1: output_connection_offset - 2 - 13 / 2 + component_width / 2,
                        y1: y,
                        x2: output_connection_offset + output_signal_id as i32 * 2,
                        y2: y,
                    });
                }
            }
        }

        for i in 0..circuit.input_signal_count {
            if is_main {
                if options.io_components {
                    circuit_data.components.push(CommonComponentData {
                        x: -8,
                        y: (i as i32 - circuit.input_signal_count as i32) * 6,
                        data: ComponentData::ToggleButton {},
                    });
                }
            } else {
                circuit_data.components.push(CommonComponentData {
                    x: -8,
                    y: (i as i32 - circuit.input_signal_count as i32) * 6 + 1,
                    data: ComponentData::Input {
                        label: i.to_string(),
                    },
                });
            }
        }

        for i in 0..circuit.output_signal_count {
            if is_main {
                if options.io_components {
                    circuit_data.components.push(CommonComponentData {
                        x: (circuit.input_signal_count + circuit.output_signal_count) as i32 * 2 + 1,
                        y: (i as i32 - circuit.output_signal_count as i32) * 6,
                        data: ComponentData::LED {
                            off_color: "#888".to_string(),
                            on_color: "#e00".to_string()
                        },
                    });
                }
            } else {
                circuit_data.components.push(CommonComponentData {
                    x: (circuit.input_signal_count + circuit.output_signal_count) as i32 * 2 + 1,
                    y: (i as i32 - circuit.output_signal_count as i32) * 6 + 1,
                    data: ComponentData::Output {
                        label: i.to_string(),
                    },
                });
            }
        }

        for i in 0..circuit.signal_count {
            top_offset += 1;

            circuit_data.connections.push(ConnectionData {
                x1: i as i32 * 2,
                y1: top_offset,
                x2: output_connection_offset + i as i32 * 2,
                y2: top_offset,
            });

            top_offset += 1;
        }

        for i in 0..circuit.signal_count {
            circuit_data.connections.push(ConnectionData {
                x1: i as i32 * 2,
                y1: 0,
                x2: i as i32 * 2,
                y2: top_offset,
            });

            circuit_data.connections.push(ConnectionData {
                x1: output_connection_offset + i as i32 * 2,
                y1: 0,
                x2: output_connection_offset + i as i32 * 2,
                y2: top_offset,
            });
        }

        if options.io_components || !is_main {
            for i in 0..circuit.input_signal_count {
                let x = i as i32 * 2;
                let y = (i as i32 - circuit.input_signal_count as i32) * 6 + 2;

                circuit_data.connections.push(ConnectionData {
                    x1: -2,
                    y1: y,
                    x2: x,
                    y2: y,
                });

                circuit_data.connections.push(ConnectionData {
                    x1: x,
                    y1: y,
                    x2: x,
                    y2: 0,
                });
            }

            for i in 0..circuit.output_signal_count {
                let x = (circuit.input_signal_count + i) as i32 * 2;
                let y = (i as i32 - circuit.output_signal_count as i32) * 6 + 2;

                circuit_data.connections.push(ConnectionData {
                    x1: x,
                    y1: y,
                    x2: (circuit.input_signal_count + circuit.output_signal_count) as i32 * 2,
                    y2: y,
                });

                circuit_data.connections.push(ConnectionData {
                    x1: x,
                    y1: y,
                    x2: x,
                    y2: 0,
                });
            }
        }

        Ok(circuit_data)
    }
}
