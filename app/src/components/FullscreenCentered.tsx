import { IonContent } from "@ionic/react";
import { ReactNode } from "react";
import style from "./FullscreenCentered.module.css";

interface FullscreenCenteredProps {
  children?: ReactNode;
}

export default function FullscreenCentered({
  children,
}: FullscreenCenteredProps) {
  return (
    <IonContent fullscreen>
      <div className={style["fullscreen-centered"]}>{children}</div>
    </IonContent>
  );
}
