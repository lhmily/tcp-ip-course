(() => {
  "use strict";

  document.querySelectorAll("[data-course-select]").forEach((root) => {
    const buttons = Array.from(root.querySelectorAll("[data-course-option]"));
    const panels = Array.from(root.querySelectorAll("[data-course-panel]"));
    if (!buttons.length || !panels.length) return;

    const select = (id, focus = false) => {
      buttons.forEach((button) => {
        const active = button.dataset.courseOption === id;
        button.setAttribute("aria-pressed", String(active));
        button.tabIndex = active ? 0 : -1;
        if (active && focus) button.focus({ preventScroll: true });
      });
      panels.forEach((panel) => {
        panel.hidden = panel.dataset.coursePanel !== id;
      });
    };

    buttons.forEach((button, index) => {
      button.addEventListener("click", () => select(button.dataset.courseOption));
      button.addEventListener("keydown", (event) => {
        let target = index;
        if (event.key === "ArrowLeft") target -= 1;
        else if (event.key === "ArrowRight") target += 1;
        else if (event.key === "Home") target = 0;
        else if (event.key === "End") target = buttons.length - 1;
        else return;
        event.preventDefault();
        target = Math.max(0, Math.min(target, buttons.length - 1));
        select(buttons[target].dataset.courseOption, true);
      });
    });

    root.classList.add("is-enhanced");
    select(buttons[0].dataset.courseOption);
  });
})();
