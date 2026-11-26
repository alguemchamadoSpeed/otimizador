import { clsx, type ClassValue } from "clsx";
import { twMerge } from "tailwind-merge";
export function cn(...inputs: ClassValue[]) { return twMerge(clsx(inputs)); }
export function formatBytes(input: number) {
  if (!Number.isFinite(input) || input <= 0) return "0 B";
  const units = ["B","KB","MB","GB","TB"];
  let value=input, unit=0;
  while(value>=1024 && unit<units.length-1){value/=1024;unit++;}
  return value.toFixed(unit===0?0:1)+" "+units[unit];
}