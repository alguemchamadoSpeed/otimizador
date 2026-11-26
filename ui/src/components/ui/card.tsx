import * as React from "react";
import { cn } from "@/lib/utils";
export function Card({className,...props}:React.HTMLAttributes<HTMLDivElement>){
 return <div className={cn("rounded-xl border border-zinc-800/90 bg-[#101014] text-zinc-100 shadow-sm",className)} {...props}/>;
}
export function CardHeader({className,...props}:React.HTMLAttributes<HTMLDivElement>){
 return <div className={cn("flex flex-col gap-1.5 p-5",className)} {...props}/>;
}
export function CardTitle({className,...props}:React.HTMLAttributes<HTMLHeadingElement>){
 return <h3 className={cn("text-sm font-semibold tracking-tight text-zinc-100",className)} {...props}/>;
}
export function CardDescription({className,...props}:React.HTMLAttributes<HTMLParagraphElement>){
 return <p className={cn("text-xs leading-relaxed text-zinc-500",className)} {...props}/>;
}
export function CardContent({className,...props}:React.HTMLAttributes<HTMLDivElement>){
 return <div className={cn("px-5 pb-5",className)} {...props}/>;
}
export function CardFooter({className,...props}:React.HTMLAttributes<HTMLDivElement>){
 return <div className={cn("flex items-center px-5 pb-5",className)} {...props}/>;
}