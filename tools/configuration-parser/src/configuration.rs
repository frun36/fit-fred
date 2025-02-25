use std::{
    collections::{BTreeMap, HashMap},
    error::Error,
    fs::File,
    io::{BufWriter, Write},
};

use colored::Colorize;
use sql::ConfigurationParametersEntry;

use crate::{
    boards::{get_board_type, BoardMap, BOARD_NAMES},
    error::Error as CrateError,
};

type InputMap = BTreeMap<String, BTreeMap<u32, u32>>;

mod sql;

pub struct Configuration {
    pub name: String,
    pub input_map: InputMap, // Sorted by board name and register address
}

impl Configuration {
    pub fn read_file(path: &str) -> Result<Self, Box<dyn Error>> {
        let mut map = InputMap::new();
        let config = ini::ini!(path);
        for (key, board_config) in config.iter() {
            let board_name = Self::parse_board_name(key)?;

            map.insert(board_name, Self::parse_board_config(board_config)?);
        }

        let name = std::path::Path::new(path)
            .file_stem()
            .ok_or(CrateError::msg("No configuration name".to_string()))?
            .to_str()
            .ok_or(CrateError::msg(
                "Configuration name is invalid UTF-8".to_string(),
            ))?
            .to_string();

        println!("Loaded configuration '{name}'");

        Ok(Self {
            name,
            input_map: map,
        })
    }

    fn parse_board_name(board_name: &str) -> Result<String, Box<dyn Error>> {
        let board_name = board_name.to_uppercase();
        if !BOARD_NAMES.contains(&board_name.as_str()) {
            return Err(Box::new(CrateError::msg(format!(
                "Board name '{board_name}' is invalid"
            ))));
        }

        let board_name = if board_name.contains("TCM") {
            "TCM0".to_string()
        } else {
            board_name
        };

        Ok(board_name)
    }

    fn parse_board_config(
        board_config: &HashMap<String, Option<String>>,
    ) -> Result<BTreeMap<u32, u32>, Box<dyn Error>> {
        let mut map = BTreeMap::new();

        for (register, value) in board_config.iter() {
            let address = u32::from_str_radix(&register[register.len() - 2..], 16)?;
            let value = u32::from_str_radix(
                value.as_ref().ok_or(Box::new(CrateError::msg(
                    "Missing register value".to_string(),
                )))?,
                16,
            )?;

            map.insert(address, value);
        }

        Ok(map)
    }

    fn convert_board_config(
        &self,
        board_map: &BoardMap,
        value_map: &BTreeMap<u32, u32>,
    ) -> Result<BoardMap, CrateError> {
        let mut board_map = board_map.clone();

        for (address, value) in value_map {
            match board_map.map.get_mut(address) {
                Some(reg) => reg.value = Some(*value),
                None => {
                    return Err(CrateError::msg(format!(
                        "Unexpected address 0x{:04X}",
                        address
                    )))
                }
            }
        }

        Ok(board_map)
    }

    fn get_converted(
        &self,
        tcm_map: &BoardMap,
        pm_map: &BoardMap,
    ) -> Result<BTreeMap<String, BoardMap>, CrateError> {
        let converted = self
            .input_map
            .iter()
            .map(|(board_name, value_map)| {
                let board_map = if get_board_type(board_name) == "TCM" {
                    tcm_map
                } else {
                    pm_map
                };

                let board_config_result = self.convert_board_config(board_map, value_map);

                board_config_result.map(|board_config| (board_name.clone(), board_config))
            })
            .collect::<Result<BTreeMap<String, BoardMap>, CrateError>>()?;

        Ok(converted)
    }

    pub fn convert_and_save_sql(
        &self,
        tcm_map: &BoardMap,
        pm_map: &BoardMap,
        author: &str,
        output_dir: &str
    ) -> Result<(), Box<dyn Error>> {
        let path = format!("{output_dir}/{}.sql", self.name);
        println!("Saving configuration '{}' into '{}'", self.name, path);
        let f = File::create(path)?;
        let mut buff = BufWriter::new(f);

        writeln!(
            buff,
            "INSERT INTO configurations (configuration_name, author, comments) VALUES ('{}', '{}', 'Control Server configuration, parsed with configuration-parser');",
            self.name, author
        )?;
        writeln!(buff)?;

        let all = self.get_converted(tcm_map, pm_map)?;

        for (board_name, board_map) in all.iter() {
            let board_type = get_board_type(board_name);

            let statements = ConfigurationParametersEntry::parse_board(
                board_map,
                &self.name,
                board_name,
                &board_type,
            )?;

            for s in statements {
                writeln!(buff, "{}", s.get_insert_statement())?;
            }

            writeln!(buff)?;
        }

        println!("{}: Configuration '{}' done", "Ok".green(), self.name);

        Ok(())
    }
}
