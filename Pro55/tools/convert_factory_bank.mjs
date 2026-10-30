import fs from "node:fs";
import path from "node:path";

if (process.argv.length !== 4) {
    console.error("usage: node convert_factory_bank.mjs presetBank.js Factory.pro55bank");
    process.exit(2);
}

const input = path.resolve(process.argv[2]);
const output = path.resolve(process.argv[3]);
let source = fs.readFileSync(input, "utf8")
    .replace("export class PresetBank", "class PresetBank")
    .replaceAll("export function ", "function ");

const bank = Function(`${source}\nreturn new PresetBank();`)();
const presets = [...bank.bank].map(([legacyId, values]) => {
    const { PatchName, ...parameters } = values;
    return {
        id: String(legacyId),
        name: PatchName,
        parameters,
    };
});

const document = {
    format: "pro55-bank",
    version: 1,
    id: "factory",
    name: "Factory",
    factory: true,
    presets,
};

fs.mkdirSync(path.dirname(output), { recursive: true });
fs.writeFileSync(output, `${JSON.stringify(document, null, 2)}\n`);
console.log(`wrote ${presets.length} presets to ${output}`);
