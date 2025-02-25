use std::{cmp::Ordering, error::Error};

use crate::error::Error as CrateError;

#[derive(Debug, serde::Deserialize)]
pub struct ParameterInfoRaw {
    pub base_address: String,
    pub start_bit: u8,
    pub end_bit: u8,
    pub is_signed: char,
    pub param_name: String,
}

#[derive(Debug, PartialEq, Eq, Clone)]
pub struct ParameterInfo {
    pub base_address: u32,
    pub start_bit: u8,
    pub end_bit: u8,
    pub is_signed: bool,
    pub param_name: String,
}

impl ParameterInfo {
    pub fn new(
        base_address: u32,
        start_bit: u8,
        end_bit: u8,
        is_signed: bool,
        param_name: String,
    ) -> Self {
        Self {
            base_address,
            start_bit,
            end_bit,
            is_signed,
            param_name,
        }
    }

    pub fn get_electronic_value(&self, reg_value: u32) -> f64 {
        /*
        0060,0,13,N,TRIGGER_5_SIGNATURE,{TRIGGER_5_SIGNATURE}>>7
        0062,0,13,N,TRIGGER_4_SIGNATURE,{TRIGGER_4_SIGNATURE}>>7
        0064,0,13,N,TRIGGER_2_SIGNATURE,{TRIGGER_2_SIGNATURE}>>7
        0066,0,13,N,TRIGGER_1_SIGNATURE,{TRIGGER_1_SIGNATURE}>>7
        0068,0,13,N,TRIGGER_3_SIGNATURE,{TRIGGER_3_SIGNATURE}>>7
        */
        if self.param_name.starts_with("TRIGGER_") && self.param_name.ends_with("_SIGNATURE") {
            return (reg_value >> 7) as f64;
        }


        let bit_len = self.end_bit - self.start_bit + 1;

        if self.is_signed {
            let sign = -(((reg_value >> self.end_bit) & 1) as f64 * (1u32 << (bit_len - 1)) as f64);
            let val = (reg_value >> self.start_bit) & ((1u32 << (bit_len - 1)) - 1);
            sign + val as f64
        } else if bit_len == 32 {
            reg_value as f64
        } else {
            let shifted = reg_value >> self.start_bit;
            let mask = (1u32 << bit_len) - 1;
            (shifted & mask) as f64
        }
    }
}

impl TryFrom<ParameterInfoRaw> for ParameterInfo {
    type Error = Box<dyn Error>;

    fn try_from(raw: ParameterInfoRaw) -> Result<Self, Self::Error> {
        let base_address = u32::from_str_radix(&raw.base_address, 16)?;
        let is_signed = match raw.is_signed {
            'Y' => true,
            'N' => false,
            c => {
                return Err(Box::new(CrateError::msg(format!(
                    "Invalid bool character {c}"
                ))))
            }
        };

        if raw.start_bit > 31 || raw.end_bit > 31 || raw.start_bit > raw.end_bit {
            return Err(Box::new(CrateError::msg(format!(
                "Invalid bit range {}:{}",
                raw.end_bit, raw.start_bit
            ))));
        }

        Ok(ParameterInfo::new(
            base_address,
            raw.start_bit,
            raw.end_bit,
            is_signed,
            raw.param_name,
        ))
    }
}

impl PartialOrd for ParameterInfo {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for ParameterInfo {
    // Ordering that will help detect compound parameters
    fn cmp(&self, other: &Self) -> Ordering {
        if self.base_address > other.base_address {
            Ordering::Greater
        } else if self.base_address < other.base_address {
            Ordering::Less
        } else if self.start_bit > other.start_bit {
            Ordering::Greater
        } else if self.start_bit < other.start_bit {
            Ordering::Less
        } else if self.end_bit < other.end_bit {
            Ordering::Greater
        } else if self.end_bit > other.end_bit {
            Ordering::Less
        } else {
            Ordering::Equal
        }
    }
}


#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn electronic_value() {
        let signed = ParameterInfo::new(0, 0, 31, true, "SIGNED".to_string());
        assert_eq!(signed.get_electronic_value(0xffffffff), -1., "Signed");
        assert_eq!(signed.get_electronic_value(10241024), 10241024., "Signed");

        let signed_half = ParameterInfo::new(0, 0, 15, true, "SIGNED_HALF".to_string());
        assert_eq!(signed_half.get_electronic_value(0xffffffff), -1., "Signed half");
        assert_eq!(signed_half.get_electronic_value(1024), 1024., "Signed half");
        assert_eq!(signed_half.get_electronic_value(0x8001), -32767., "Signed half");

        let unsigned = ParameterInfo::new(0, 0, 31, false, "UNSIGNED".to_string());
        assert_eq!(unsigned.get_electronic_value(1296), 1296., "Unsigned");

        let unsigned_half = ParameterInfo::new(0, 16, 31, false, "UNSIGNED_HALF".to_string());
        assert_eq!(unsigned_half.get_electronic_value(0x00080008), 8., "Unsigned half");

        let unsigned_flag = ParameterInfo::new(0, 0, 0, false, "UNSIGNED_FLAG".to_string());
        assert_eq!(unsigned_flag.get_electronic_value(1), 1., "Unsigned flag");
        assert_eq!(unsigned_flag.get_electronic_value(0), 0., "Unsigned flag");

        let signed_flag = ParameterInfo::new(0, 0, 0, true, "SIGNED_FLAG".to_string());
        assert_eq!(signed_flag.get_electronic_value(1), -1., "Signed flag");
        assert_eq!(signed_flag.get_electronic_value(0), 0., "Signed flag");

        let unsigned_middle = ParameterInfo::new(0, 8, 23, false, "UNSIGNED_MIDDLE".to_string());
        assert_eq!(unsigned_middle.get_electronic_value(0x00123400), 4660., "Unsigned middle");
        
        let signed_middle = ParameterInfo::new(0, 8, 23, true, "SIGNED_MIDDLE".to_string());
        assert_eq!(signed_middle.get_electronic_value(0x00823400), -32204., "Signed middle");
    }
}