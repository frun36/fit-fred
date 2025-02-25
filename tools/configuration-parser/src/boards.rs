use parameter_info::{ParameterInfo, ParameterInfoRaw};
use register::Register;

use std::{collections::BTreeMap, error::Error, fs::File, io::BufReader};

pub mod parameter_info;
pub mod register;

pub const BOARD_NAMES: [&str; 21] = [
    "TCM", "PMA0", "PMA1", "PMA2", "PMA3", "PMA4", "PMA5", "PMA6", "PMA7", "PMA8", "PMA9", "PMC0",
    "PMC1", "PMC2", "PMC3", "PMC4", "PMC5", "PMC6", "PMC7", "PMC8", "PMC9",
];

pub fn get_board_type(board_name: &str) -> String {
    if board_name.contains("TCM") {
        "TCM".to_string()
    } else {
        "PM".to_string()
    }
}

#[derive(Clone)]
pub struct BoardMap {
    pub map: BTreeMap<u32, Register>,
}

impl BoardMap {
    pub fn read_file(path: &str) -> Result<BoardMap, Box<dyn Error>> {
        let f = File::open(path)?;
        let reader = BufReader::new(f);
        let mut reader = csv::Reader::from_reader(reader);

        let mut map: BTreeMap<u32, Register> = BTreeMap::new();

        for line in reader.deserialize() {
            let raw: ParameterInfoRaw = line?;
            let p: ParameterInfo = raw.try_into()?;

            let reg = match map.get_mut(&p.base_address) {
                Some(r) => r,
                None => {
                    map.insert(p.base_address, Register::new(p.base_address));
                    map.get_mut(&p.base_address).unwrap()
                }
            };

            if let Err(e) = reg.insert_parameter(p) {
                println!("{e}");
            }
        }

        Ok(BoardMap { map })
    }
}
