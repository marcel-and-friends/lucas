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
    <IonContent fullscreen>
      <div
        className={
          "flex h-full w-full flex-col items-center justify-center " + className
        }
      >
        {children}
      </div>
    </IonContent>
  );
}
