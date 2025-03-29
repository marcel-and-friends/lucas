import { IonTab } from "@ionic/react";
import { ReactNode } from "react";
import style from "./Tab.module.css";

interface TabProps {
  tab: string;
  children?: ReactNode;
}

export default function Tab({ tab, children }: TabProps) {
  return (
    <IonTab tab={tab}>
      <div id={`${tab}-page`} className={style["tab-container"]}>
        {children}
      </div>
    </IonTab>
  );
}
