import axios from "axios";
import { CompileError, InternalError } from "../utils/error";
import { MemInfo } from "../utils/type";

type MemoryType = 'iram' | 'dram' | 'iflash' | 'dflash';

export type MemoryUpdate = {
  blocks: {type: MemoryType, address: number, data: string}[],
  entryPoints: {id: number, address: number}[]
}

export type CompileResult = {
    result: MemoryUpdate,
    compileTime: number
}

export async function compile(id: number, src: string): Promise<CompileResult> {
  return post("compile", {id, src});
}

export async function interactiveCompile(id:number, src: string): Promise<CompileResult> {
  return post("interactive-compile", {id, src});
}

export async function compileWithProfiling(id: number, src: string): Promise<CompileResult> {
  return post("compile-with-profiling", {id, src}); 
}

export async function jitCompile(funcId: number, paramTypes: string[]): Promise<CompileResult> {
  return post("jit-compile", {funcId, paramTypes});
}

export async function reset(memInfo:MemInfo) {
    return post("reset", memInfo);
}

async function post(path: string, body: object) {
  const baseURL = "http://localhost:8080/";
  const headers = {
    "Content-Type": "application/json",
    "Access-Control-Allow-Origin": "*",
  };
  try {
    const response = await axios.post(
    baseURL + path,
    body,
    { headers }
  );
  return JSON.parse(response.data);
  } catch (e) {
    if (e instanceof axios.AxiosError) {
      if (e.response?.status === CompileError.errorCode) {
        throw new CompileError(JSON.parse(e.response?.data).message.messages)
      }
      if (e.response?.status === InternalError.errorCode) {
        throw new InternalError(JSON.parse(e.response?.data).message.message)
      }
    }
    throw e;
  }
}
