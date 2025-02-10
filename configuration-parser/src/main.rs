use std::{
    error::Error,
    fs::{self, DirEntry},
};

use boards::BoardMap;
use configuration::Configuration;

use clap::Parser;

use colored::Colorize;

mod boards;
mod configuration;
mod error;

#[derive(Parser, Debug)]
#[command(
    name = "Configuration parser",
    version = "1.0",
    about = "Parses configuration files from Control Server into SQL INSERT statements for the FRED DB"
)]
struct Cli {
    #[arg(
        short,
        long,
        help = "All files from this directory will be parsed"
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

fn parse_one(
    dir_entry: DirEntry,
    tcm_map: &BoardMap,
    pm_map: &BoardMap,
    author: &str,
    output_dir: &str,
) -> Result<(), Box<dyn Error>> {
    let path_buf = dir_entry.path();
    let path = path_buf.to_str().ok_or(crate::error::Error::msg(
        "Configuration file path is not valid UTF-8".to_string(),
    ))?;

    let configuration = Configuration::read_file(path)?;
    configuration.convert_and_save_sql(tcm_map, pm_map, author, output_dir)?;
    
    println!();

    Ok(())
}

fn main() -> Result<(), Box<dyn Error>> {
    let args = Cli::parse();

    let tcm_map = BoardMap::read_file(&args.tcm_params).expect("Invalid TCM map file");
    let pm_map = BoardMap::read_file(&args.pm_params).expect("Invalid PM map file");

    for dir_entry in fs::read_dir(args.input_dir)? {
        if let Err(e) = parse_one(dir_entry?, &tcm_map, &pm_map, &args.author, &args.output_dir) {
            println!("{}: {e}", "Error".red());
            println!();
        }
    }

    Ok(())
}
