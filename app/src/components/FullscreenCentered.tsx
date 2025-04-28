import { cn } from "@/lib/utils";
import { IonContent } from "@ionic/react";
import { ReactNode } from "react";

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
