
use std::borrow::Borrow;
use std::rc::{Rc, Weak};
use std::fs::File;
use std::io::{BufWriter, BufReader};
use std::cmp;
use std::path::PathBuf;

use serde::{Serialize, Deserialize};
use serde_json;
use matches::matches;

use crate::shared::intermediate::{IntermediateBlock, IntermediateOp, Intermediate};
use crate::backend;
use crate::backend::{Error, ErrorKind};

struct Options {
    io_components: bool,
    testbench: Option<PathBuf>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
struct ConnectionData {
    x1: i32,
    y1: i32,
    x2: i32,
    y2: i32,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
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
    #[serde(rename = "halfadder")]
    HalfAdder {},
    #[serde(rename = "fulladder")]
    FullAdder {},
    #[serde(rename = "dflipflop")]
    FlipFlop {},
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

    #[serde(rename = "clock")]
    Clock { period: u32 },
    #[serde(rename = "bargraph")]
    BarGraph { size: u32, #[serde(rename = "offColor")] off_color: String, #[serde(rename = "onColor")] on_color: String },
    #[serde(rename = "register")]
    Register { width: u32 },
    #[serde(rename = "sram")]
    SRAM { addresswidth: u32, datawidth: u32 },
    #[serde(rename = "rom")]
    ROM { addresswidth: u32, wordsize: u32, contents: String },
}

#[derive(Clone, Debug, Serialize, Deserialize)]
struct CommonComponentData {
    x: i32,
    y: i32,

    #[serde(flatten)]
    data: ComponentData,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
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
    Const0,
    Const1,
    AND,
    OR,
    XOR,
    NOT,
    HalfAdder,
    FullAdder,
    FlipFlop,

    Circuit(String),
}

struct Component {
    typ: ComponentType,
    inputs: Vec<u32>,
    outputs: Vec<u32>,

    width: i32,
    height: i32,
    input_pin_positions: Vec<(i32, i32)>,
    output_pin_positions: Vec<(i32, i32)>,
}

impl Component {
    fn new(typ: ComponentType, inputs: Vec<u32>, outputs: Vec<u32>) -> Component {
        let width;
        let height;
        let mut input_pin_positions;
        let mut output_pin_positions;

        match &typ {
            ComponentType::Const0 |
            ComponentType::Const1 => {
                width = 5;
                height = 4;
                input_pin_positions = vec![];
                output_pin_positions = vec![(6, 2)];
            },
            ComponentType::AND |
            ComponentType::OR => {
                width = 5;
                height = 1 + (inputs.len() as i32 - 1) * 2 + 1;

                let mid = height / 2;
                let input_top_offset = 1 + mid - inputs.len() as i32;
                let output_top_offset = 1 + mid - outputs.len() as i32;

                input_pin_positions = Vec::new();
                for i in 0..inputs.len() {
                    input_pin_positions.push((-1, input_top_offset + i as i32 * 2));
                }

                output_pin_positions = Vec::new();
                for i in 0..outputs.len() {
                    output_pin_positions.push((width + 1, output_top_offset + i as i32 * 2));
                }
            },
            ComponentType::XOR => {
                width = 5;
                height = 4;
                input_pin_positions = vec![(-1, 1), (-1, 3)];
                output_pin_positions = vec![(6, 2)];
            },
            ComponentType::NOT => {
                width = 5;
                height = 4;
                input_pin_positions = vec![(-1, 2)];
                output_pin_positions = vec![(6, 2)];
            },
            ComponentType::HalfAdder => {
                width = 5;
                height = 4;
                input_pin_positions = vec![(-1, 1), (-1, 3)];
                output_pin_positions = vec![(6, 1), (6, 3)];
            },
            ComponentType::FullAdder => {
                width = 5;
                height = 6;
                input_pin_positions = vec![(-1, 1), (-1, 3), (-1, 5)];
                output_pin_positions = vec![(6, 2), (6, 4)];
            },
            ComponentType::FlipFlop => {
                width = 5;
                height = 6;
                input_pin_positions = vec![(-1, 5), (-1, 1)];
                output_pin_positions = vec![(6, 1)];
            },
            ComponentType::Circuit(_) => {
                let max_pins = cmp::max(inputs.len(), outputs.len()) as i32;

                width = 11;
                height = cmp::max(4, 1 + (max_pins - 1) * 2 + 1);

                let mid = height / 2;
                let input_top_offset = 1 + mid - inputs.len() as i32;
                let output_top_offset = 1 + mid - outputs.len() as i32;

                input_pin_positions = Vec::new();
                for i in 0..inputs.len() {
                    input_pin_positions.push((-1, input_top_offset + i as i32 * 2));
                }

                output_pin_positions = Vec::new();
                for i in 0..outputs.len() {
                    output_pin_positions.push((width + 1, output_top_offset + i as i32 * 2));
                }
            },
        }

        Component {
            typ,
            inputs,
            outputs,
            width,
            height,
            input_pin_positions,
            output_pin_positions,
        }
    }
}

struct Connection {
    input: u32,
    output: u32,
}

impl Connection {
    fn new(input: u32, output: u32) -> Connection {
        Connection {
            input,
            output,
        }
    }
}

enum CircuitElement {
    Connection(Connection),
    Component(Component),
}

struct Circuit<'a> {
    input_signal_count: u32,
    output_signal_count: u32,
    signal_count: u32,
    elements: Vec<CircuitElement>,
    input_signal_names: &'a Vec<String>,
    output_signal_names: &'a Vec<String>,
}

impl<'a> Circuit<'a> {
    fn allocate_signals(&mut self, count: u32) -> u32 {
        let base_signal = self.signal_count;
        self.signal_count += count;
        base_signal
    }
}

pub struct LogicSimulator {
    //
}

enum CommandLineOption {
    None,
    Testbench,
}

impl LogicSimulator {
    pub fn run(output_path: Option<&str>, blocks: &Intermediate, args: &[&str]) -> backend::Result {
        let mut options = Options {
            io_components: true,
            testbench: None,
        };

        let mut current_option = CommandLineOption::None;

        for &arg in args {
            match current_option {
                CommandLineOption::None => match arg {
                    "--no-io-components" => options.io_components = false,
                    "--testbench" => current_option = CommandLineOption::Testbench,
                    arg => return Err(ErrorKind::Custom(format!("unrecognized backend option '{}'", arg)).into()),
                },
                CommandLineOption::Testbench => {
                    options.testbench = Some(PathBuf::from(arg));

                    current_option = CommandLineOption::None;
                },
            }
        }

        if let CommandLineOption::None = current_option {
            //
        } else {
            return Err(ErrorKind::Custom("missing value for last option".to_string()).into());
        }

        let output_path = output_path.unwrap_or("circuit.json");

        let mut circuits = Self::generate_circuits(blocks, &options)?;

        if let Some(testbench_path) = options.testbench {
            let testbench_file = File::open(testbench_path)
                .map_err(|e| Error::with_source(ErrorKind::Custom("unable to open testbench file".to_string()), e))?;
            let testbench_reader = BufReader::new(testbench_file);

            let testbench_data: FileData = serde_json::from_reader(testbench_reader)
                .map_err(|e| Error::with_source(ErrorKind::Custom("unable to read testbench file".to_string()), e))?;

            for circuit in &testbench_data.circuits {
                if circuit.name == "main" {
                    let main_circuit = circuits.iter_mut()
                        .find(|circuit| circuit.name == "main")
                        .unwrap();

                    for component in &circuit.components {
                        main_circuit.components.push((*component).clone());
                    }

                    for connection in &circuit.connections {
                        main_circuit.connections.push((*connection).clone());
                    }
                } else {
                    circuits.push((*circuit).clone());
                }
            }
        }

        let data = FileData {
            version: 1,
            circuits,
        };

        let f = File::create(output_path)
            .map_err(|e| Error::with_source(ErrorKind::Custom(format!("Unable to create output file: {}", e)), e))?;
        let writer = BufWriter::new(f);

        serde_json::to_writer(writer, &data)
            .map_err(|e| Error::with_source(ErrorKind::Custom(format!("Unable to write JSON: {}", e)), e))?;

        Ok(())
    }

    fn generate_circuits(blocks: &Vec<Rc<IntermediateBlock>>, options: &Options) -> Result<Vec<CircuitData>, Error> {
        let main_found = blocks.iter()
            .any(|block| {
                let block: &IntermediateBlock = block.borrow();
                block.name == "main"
            });

        if !main_found {
            return Err(ErrorKind::Custom("main block not defined".to_string()).into());
        }

        let mut circuits = Vec::new();

        for block in blocks {
            let block = block.borrow();
            circuits.push(Self::generate_circuit(block, options, block.name == "main")?);
        }

        Ok(circuits)
    }

    fn generate_circuit(block: &IntermediateBlock, options: &Options, is_main: bool) -> Result<CircuitData, Error> {
        let io_signal_count = block.input_signal_count + block.output_signal_count;

        let mut circuit = Circuit {
            input_signal_count: block.input_signal_count,
            output_signal_count: block.output_signal_count,
            signal_count: io_signal_count,
            elements: Vec::new(),
            input_signal_names: &block.input_signal_names,
            output_signal_names: &block.output_signal_names,
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

            let mut inputs = Vec::new();
            let mut outputs = Vec::new();

            for j in 0..nested_block.input_signal_count {
                inputs.push(nested_blocks_base_signal + j);
            }

            for j in 0..nested_block.output_signal_count {
                outputs.push(nested_blocks_base_signal + nested_block.input_signal_count + j);
            }

            circuit.elements.push(CircuitElement::Component(Component::new(
                ComponentType::Circuit(nested_block.name.clone()),
                inputs,
                outputs
            )));
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
            if let IntermediateOp::Connect = stmt.op {
                circuit.elements.push(CircuitElement::Connection(Connection::new(stmt.input_signal_ids[0], stmt.output_signal_ids[0])));
            } else {
                let typ = match stmt.op {
                    IntermediateOp::Connect => panic!(),
                    IntermediateOp::Const0 => ComponentType::Const0,
                    IntermediateOp::Const1 => ComponentType::Const1,
                    IntermediateOp::AND => ComponentType::AND,
                    IntermediateOp::OR => ComponentType::OR,
                    IntermediateOp::XOR => ComponentType::XOR,
                    IntermediateOp::NOT => ComponentType::NOT,
                    IntermediateOp::Add => match stmt.size {
                        2 => ComponentType::HalfAdder,
                        3 => ComponentType::FullAdder,
                        _ => panic!(),
                    },
                    IntermediateOp::FlipFlop => ComponentType::FlipFlop,
                    _ => return Err(ErrorKind::Custom(format!("unsupported op {:?} in intermediate statement", stmt.op)).into()),
                };

                let mut inputs = Vec::new();
                let mut outputs = Vec::new();

                for &input_signal_id in &stmt.input_signal_ids {
                    inputs.push(input_signal_id);
                }

                for &output_signal_id in &stmt.output_signal_ids {
                    outputs.push(output_signal_id);
                }

                circuit.elements.push(CircuitElement::Component(Component::new(typ, inputs, outputs)));
            }
        }

        Self::output_circuit(&circuit, &block.name, options, is_main)
    }

    fn output_circuit(circuit: &Circuit, name: &str, options: &Options, is_main: bool) -> Result<CircuitData, Error> {
        let mut circuit_data = CircuitData {
            name: name.to_string(),
            label: name.to_string(),
            components: Vec::new(),
            connections: Vec::new(),
        };

        let component_offset = (circuit.signal_count as i32 - 1) * 2 + 2;
        let output_connection_offset = component_offset + 13 + 2;

        let mut top_offset = 0;

        for element in &circuit.elements {
            top_offset += 1;

            match element {
                CircuitElement::Connection(connection) => {
                    circuit_data.connections.push(ConnectionData {
                        x1: connection.input as i32 * 2,
                        y1: top_offset,
                        x2: output_connection_offset + connection.output as i32 * 2,
                        y2: top_offset,
                    });
                },
                CircuitElement::Component(component) => {
                    let data = match &component.typ {
                        ComponentType::Const0 | ComponentType::Const1 => ComponentData::Const {
                            value: matches!(component.typ, ComponentType::Const1),
                        },
                        ComponentType::AND => ComponentData::AND {
                            inputs: component.inputs.len() as u32,
                        },
                        ComponentType::OR => ComponentData::OR {
                            inputs: component.inputs.len() as u32,
                        },
                        ComponentType::XOR => ComponentData::XOR {},
                        ComponentType::NOT => ComponentData::NOT {},
                        ComponentType::HalfAdder => ComponentData::HalfAdder {},
                        ComponentType::FullAdder => ComponentData::FullAdder {},
                        ComponentType::FlipFlop => ComponentData::FlipFlop {},
                        ComponentType::Circuit(name) => ComponentData::Custom {
                            name: name.clone(),
                        },
                    };

                    circuit_data.components.push(CommonComponentData {
                        x: component_offset + 13 / 2 - component.width / 2,
                        y: top_offset,
                        data
                    });

                    top_offset += component.height;
                },
            }

            top_offset += 1;

            if let CircuitElement::Component(component) = element {
                let component_top_offset = top_offset - 1 - component.height;

                for (i, &input_signal_id) in component.inputs.iter().enumerate() {
                    let pos = component.input_pin_positions[i];

                    let y = component_top_offset + pos.1;

                    circuit_data.connections.push(ConnectionData {
                        x1: input_signal_id as i32 * 2,
                        y1: y,
                        x2: component_offset + 13 / 2 - component.width / 2 + pos.0,
                        y2: y,
                    });
                }

                for (i, &output_signal_id) in component.outputs.iter().enumerate() {
                    let pos = component.output_pin_positions[i];

                    let y = component_top_offset + pos.1;

                    circuit_data.connections.push(ConnectionData {
                        x1: component_offset + 13 / 2 - component.width / 2 + pos.0,
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
                        label: circuit.input_signal_names[i as usize].clone(),
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
                        label: circuit.output_signal_names[i as usize].clone(),
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
