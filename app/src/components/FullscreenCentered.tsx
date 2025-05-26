import { IonContent } from "@ionic/react";
import type { ReactNode } from "react";
import { cn } from "@/lib/utils";

interface FullscreenCenteredProps {
  children?: ReactNode;
  className?: string;
}

export default function FullscreenCentered({
  children,
  className,
}: FullscreenCenteredProps) {
  return (
    <IonContent>
      <div
        className={cn(
          "flex h-full w-full flex-col items-center justify-center",
          className,
        )}
      >
        {children}
      </div>
    </IonContent>
  );
}
