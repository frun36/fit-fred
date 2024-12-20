use std::{error::Error, fs};

use boards::BoardMap;
use configuration::Configuration;

use clap::Parser;

mod boards;
mod configuration;
mod error;

#[derive(Parser, Debug)]
#[command(
    name = "Configuration parser",
    version = "1.0",
    about = "Parses .cfg files from Control Server into SQL INSERT statements for the FRED DB"
)]
struct Cli {
    #[arg(
        short,
        long,
        help = "All .cfg files from this directory will be parsed"
    )]
    input_dir: String,

    #[arg(short, long, help = "output directory (must exist)", default_value_t = String::from("sql"))]
    output_dir: String,

    #[arg(short, long)]
    author: String,

    #[arg(long, help = ".csv with description of TCM parameters", default_value_t = String::from("tcm.csv"))]
    tcm_params: String,

    #[arg(long, help = ".csv with description of PM parameters", default_value_t = String::from("pm.csv"))]
    pm_params: String,
}

fn main() -> Result<(), Box<dyn Error>> {
    let args = Cli::parse();

    let tcm_map = BoardMap::read_file(&args.tcm_params).expect("Invalid TCM map file");
    let pm_map = BoardMap::read_file(&args.pm_params).expect("Invalid PM map file");

    for dir_entry in fs::read_dir(args.input_dir)? {
        let path_buf = dir_entry?.path();
        let path = path_buf.to_str().ok_or(crate::error::Error::msg(
            "Configuration file path is not valid UTF-8".to_string(),
        ))?;
        let configuration = Configuration::read_file(path)?;

        configuration
            .convert_and_save_sql(&tcm_map, &pm_map, &args.author)
            .expect("Couldn't convert or save SQL");

        println!();
    }

    Ok(())
}
