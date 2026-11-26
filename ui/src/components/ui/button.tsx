import * as React from "react";
import { cva, type VariantProps } from "class-variance-authority";
import { cn } from "@/lib/utils";

const buttonVariants = cva(
  "inline-flex items-center justify-center gap-2 whitespace-nowrap rounded-md text-sm font-medium transition-colors focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-zinc-500 disabled:pointer-events-none disabled:opacity-50 [&_svg]:pointer-events-none [&_svg]:size-4 [&_svg]:shrink-0",
  { variants: {
    variant: {
      default:"bg-zinc-100 text-zinc-950 shadow-sm hover:bg-white",
      secondary:"border border-zinc-700 bg-zinc-900 text-zinc-100 hover:bg-zinc-800",
      outline:"border border-zinc-700 bg-transparent text-zinc-200 hover:bg-zinc-900",
      ghost:"text-zinc-400 hover:bg-zinc-900 hover:text-zinc-100",
      destructive:"bg-red-950/70 text-red-200 hover:bg-red-900",
      link:"text-zinc-100 underline-offset-4 hover:underline"
    },
    size:{default:"h-9 px-4 py-2",sm:"h-8 rounded-md px-3 text-xs",lg:"h-10 rounded-md px-5",icon:"size-9 p-0"}
  }, defaultVariants:{variant:"default",size:"default"} }
);

export interface ButtonProps extends React.ButtonHTMLAttributes<HTMLButtonElement>, VariantProps<typeof buttonVariants> {}
export const Button=React.forwardRef<HTMLButtonElement,ButtonProps>(({className,variant,size,type="button",...props},ref)=>(
  <button ref={ref} type={type} className={cn(buttonVariants({variant,size}),className)} {...props}/>
));
Button.displayName="Button";