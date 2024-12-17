use crate::{
    boards::{register::Register, BoardMap},
    error::Error,
};

#[derive(Clone)]
pub struct ConfigurationParametersEntry {
    configuration_name: String,
    board_name: String,
    board_type: String,
    parameter_name: String,
    parameter_electronic_value: f64,
}

impl ConfigurationParametersEntry {
    pub fn get_insert_statement(&self) -> String {
        format!(
            "INSERT INTO configuration_parameters (configuration_name, board_name, board_type, parameter_name, parameter_electronic_value)\n\tVALUES ('{}', '{}', '{}', '{}', {:.0});", 
            self.configuration_name, self.board_name, self.board_type, self.parameter_name, self.parameter_electronic_value)
    }

    fn parse_register(
        reg: &Register,
        configuration_name: &str,
        board_name: &str,
        board_type: &str,
    ) -> Result<Vec<Self>, crate::error::Error> {
        let reg_value = match reg.value {
            Some(v) => v,
            None => {
                return Err(Error::msg(format!(
                    "Register {:4X} is empty",
                    reg.base_address
                )))
            }
        };

        let vec = reg
            .parameters
            .iter()
            .map(|p| {
                let parameter_electronic_value = p.get_electronic_value(reg_value);
                let parameter_name = p.param_name.clone();
                let configuration_name = configuration_name.to_string();
                let board_name = board_name.to_string();
                let board_type = board_type.to_string();
                ConfigurationParametersEntry {
                    configuration_name,
                    board_name,
                    board_type,
                    parameter_name,
                    parameter_electronic_value,
                }
            })
            .collect();

        Ok(vec)
    }

    pub fn parse_board(
        board_map: &BoardMap,
        configuration_name: &str,
        board_name: &str,
        board_type: &str,
    ) -> Result<Vec<ConfigurationParametersEntry>, Error> {
        let result = board_map
            .map
            .values()
            .map(|reg| Self::parse_register(reg, configuration_name, board_name, board_type))
            .collect::<Result<Vec<Vec<ConfigurationParametersEntry>>, Error>>()?
            .iter()
            .flat_map(|c| c.clone().to_owned())
            .collect();

        Ok(result)
    }
}
