import * as React from "react";
import { cva, type VariantProps } from "class-variance-authority";
import { cn } from "@/lib/utils";
const badgeVariants=cva("inline-flex items-center gap-1 rounded-full border px-2.5 py-0.5 text-[11px] font-medium transition-colors",{
 variants:{variant:{
  default:"border-zinc-700 bg-zinc-800/70 text-zinc-200",
  secondary:"border-zinc-800 bg-zinc-900 text-zinc-400",
  success:"border-emerald-900/80 bg-emerald-950/50 text-emerald-300",
  warning:"border-amber-900/80 bg-amber-950/40 text-amber-300",
  outline:"border-zinc-700 bg-transparent text-zinc-300",
  destructive:"border-red-900/80 bg-red-950/40 text-red-300"
 }},defaultVariants:{variant:"default"}
});
export interface BadgeProps extends React.HTMLAttributes<HTMLSpanElement>,VariantProps<typeof badgeVariants>{}
export function Badge({className,variant,...props}:BadgeProps){
 return <span className={cn(badgeVariants({variant}),className)} {...props}/>;
}