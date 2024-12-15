use std::error::Error;

use configuration::Configuration;
use boards::BoardMap;

mod boards;
mod configuration;
mod error;

fn main() -> Result<(), Box<dyn Error>> {
    let tcm_map = BoardMap::read_file("tcm.csv")?;
    let pm_map = BoardMap::read_file("pm.csv")?;
    let configuration = Configuration::read_file("fv0/phys_18ch_05_221109.cfg")?;

    configuration.convert_and_save_sql(&tcm_map, &pm_map, "Varlen Grabski").expect("Couldn't convert or save SQL");
    Ok(())
}
