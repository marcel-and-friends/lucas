import { execSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { glob } from "glob";
import type { Plugin } from "vite";

interface ProtoGenOptions {
  protoDir: string;
  outDir: string;
}

function generateProto(
  protoDir: string,
  outDir: string,
  clean: boolean = false,
) {
  if (!fs.existsSync(outDir)) {
    fs.mkdirSync(outDir, { recursive: true });
  } else if (clean) {
    fs.rmSync(outDir, { recursive: true });
    fs.mkdirSync(outDir, { recursive: true });
  }

  const protoFiles = glob.sync("**/*.proto", {
    cwd: protoDir,
    absolute: true,
    follow: true,
  });
  if (protoFiles.length === 0) {
    console.log(`No .proto files found in ${protoDir}`);
    return;
  }

  const relativeFiles = protoFiles.map((file) => path.relative(protoDir, file));
  const command = [
    "protoc",
    `--plugin=protoc-gen-ts_proto=${path.resolve(process.cwd(), "node_modules/.bin/protoc-gen-ts_proto")}`,
    `--proto_path=${protoDir}`,
    `--ts_proto_out=${outDir}`,
    `--ts_proto_opt=removeEnumPrefix=true`,
    `--ts_proto_opt=snakeToCamel=false`,
    ...relativeFiles,
  ].join(" ");

  console.log(`Generating proto files: ${command}`);
  try {
    execSync(command, { stdio: "inherit" });
  } catch (error) {
    console.error("Failed to generate proto files:", error);
  }
}

function isWithin(outer: string, inner: string) {
  const rel = path.relative(outer, inner);
  return rel && !rel.startsWith("../") && rel !== "..";
}

export default function viteProtoGenPlugin(options: ProtoGenOptions): Plugin {
  return {
    name: "vite-plugin-proto-gen",
    buildStart() {
      generateProto(options.protoDir, options.outDir, true);
    },
    configureServer(server) {
      const handler = (filePath: string) => {
        if (isWithin(options.protoDir, filePath))
          generateProto(options.protoDir, options.outDir);
      };

      const unlinkHandler = (filePath: string) => {
        if (isWithin(options.protoDir, filePath))
          generateProto(options.protoDir, options.outDir, true);
      };

      server.watcher.add(options.protoDir);

      server.watcher.on("add", handler);
      server.watcher.on("change", handler);
      server.watcher.on("unlink", unlinkHandler);
    },
  };
}
