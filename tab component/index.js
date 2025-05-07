const tabs = document.querySelectorAll(".tab");
const contents = document.querySelectorAll(".tab-content");

tabs.forEach(tab => {
  tab.addEventListener("click", () => {
    const targetId = tab.dataset.tab;

    // remove active from all tabs
    tabs.forEach(t => t.classList.remove("active"));
    tab.classList.add("active");

    // hide all contents
    contents.forEach(content => content.classList.add("hidden"));

    // show selected tab content
    document.getElementById(targetId).classList.remove("hidden");
  });
});
