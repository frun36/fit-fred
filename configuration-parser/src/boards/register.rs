use crate::{boards::ParameterInfo, error::Error as CrateError};

#[derive(Clone, Debug)]
pub struct Register {
    pub base_address: u32,
    pub value: Option<u32>,
    pub parameters: Vec<ParameterInfo>,
}

impl Register {
    pub fn new(base_address: u32) -> Self {
        Self {
            base_address,
            value: None,
            parameters: Vec::new(),
        }
    }

    pub fn insert_parameter(&mut self, p: ParameterInfo) -> Result<(), CrateError> {
        if p.base_address != self.base_address {
            return Err(CrateError::msg(format!(
                "Parameter {} not in register {:02x}",
                p.param_name, self.base_address
            )));
        }

        let position = match self.parameters.binary_search(&p) {
            Ok(_) => {
                return Err(CrateError::msg(format!(
                    "Parameter '{}' already inserted",
                    p.param_name
                )))
            }
            Err(i) => i,
        };

        if position > 0 {
            let prev = self.parameters.get(position - 1).unwrap();
            if prev.start_bit <= p.start_bit && prev.end_bit >= p.end_bit {
                return Err(CrateError::msg(format!(
                    "Warning: parameter '{}' [{}:{}] overshadowed by '{}' [{}:{}]",
                    p.param_name,
                    p.end_bit,
                    p.start_bit,
                    prev.param_name,
                    prev.end_bit,
                    prev.start_bit,
                )));
            } else if prev.end_bit >= p.start_bit && prev.end_bit < p.end_bit {
                return Err(CrateError::msg(format!(
                    "Invalid overlap between '{}' [{}:{}] and '{}' [{}:{}]",
                    prev.param_name,
                    prev.end_bit,
                    prev.start_bit,
                    p.param_name,
                    p.end_bit,
                    p.start_bit,
                )));
            }
        }
        self.parameters.insert(position, p);
        Ok(())
    }
}
