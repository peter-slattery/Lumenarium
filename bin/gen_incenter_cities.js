const fs = require("fs");

const IN_FILE_PATH_PRIMARY   = "../run_tree/data/cities_final.json";
const IN_FILE_PATH_SECONDARY = "../run_tree/data/cities_secondary_final.json";
const OUT_FILE_PATH = "../src_v2/user_space/incenter_gen_cities.h"

function print_city_desc (city, prefix, dest, gets_own_universe) 
{
  const city_ascii = city.city_ascii
    .toLowerCase()
    .replaceAll(' ', '_')
    .replaceAll('-', '_')
    .replaceAll('\'', '')
    .replaceAll('`', '');
    
  const city_id = `${prefix}_${city_ascii}`;
  const { lat, lng } = city;
  
  dest.enum_out += `  ${city_id} = ${dest.enum_counter++},\n`;
    
  const universe = gets_own_universe ? city_id : "incenter_secondary_city_universe";

  dest.desc_out += `  [${city_id}] = {
    .id = ${city_id},
    .lat = ${lat},
    .lon = ${lng},
    .sacn_universe = ${universe},
  },\n`;

  dest.strings_out += `  [${city_id}] = "${city_id}",\n`;
}

function main () 
{
  const primary_file = fs.readFileSync(IN_FILE_PATH_PRIMARY, {});
  const primary_json = JSON.parse(primary_file);

  const secondary_file = fs.readFileSync(IN_FILE_PATH_SECONDARY, {});
  const secondary_json = JSON.parse(secondary_file);

  let out = "// NOTE: This file is autogenerated by csv_to_cstruct.js\n";
  
  let enum_counter = 0;
  let enum_out = "// NOTE: These are values for Incenter_City_Id\nenum {\n";
  enum_out    += `  city_black_rock = ${enum_counter++},\n`;
  
  let desc_out   = "global Incenter_City_Desc city_descs[] = {\n";

  let strings_out = "global char* city_strings[] = {\n";

  let dest = {
    enum_counter,
    enum_out,
    desc_out,
    strings_out,
  };

  primary_json.forEach((city) => {
    print_city_desc(city, "city", dest, true);    
  });

  // Add Black Rock City
  dest.desc_out += `\n  // Black Rock City\n  [city_black_rock] = {
    .id = city_black_rock,
    .lat = -90.0f,
    .lon = 0,
    .sacn_universe = city_black_rock,
  },\n`;

  dest.strings_out += `  [city_black_rock] = "city_black_rock",\n`;
  dest.enum_out += "  city_count,\n";
  dest.enum_out += "  city_secondary_first = city_count + 1,\n";

  secondary_json.forEach((city) => {
    print_city_desc(city, "city_secondary", dest, false);
  });
  
  dest.enum_out      += "  city_secondary_count,\n";
  dest.enum_out      += "};\n\n";
  dest.desc_out      += "};\n\n";
  dest.strings_out   += "};\n\n";
  
  out += dest.enum_out;
  out += dest.desc_out;
  out += dest.strings_out;
  fs.writeFileSync(OUT_FILE_PATH, out, {});
}

main();
