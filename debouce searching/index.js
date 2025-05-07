const fruits = [
  "Apple", "Banana", "Orange", "Mango", "Papaya",
  "Pineapple", "Grapes", "Kiwi", "Strawberry", "Watermelon"
];


const searchInput = document.getElementById('searchInput')
const resultList = document.getElementById("resultList")
let debounceTimer = null;

function renderResults(filtered){
  resultList.innerHTML = ''
  if(filtered.length === 0) {
    resultList.innerHTML = '<li>No results found</li>'
    return
  }

  filtered.forEach(fruit => {
    const li = document.createElement('li')
    li.textContent = fruit;
    resultList.appendChild(li)
  })
}

function handleSearch(query) {
  const filtered = fruits.filter(fruit => fruit.toLowerCase().includes(query.toLowerCase()))
  renderResults(filtered)
}

searchInput.addEventListener('input', ()=> {
  clearTimeout(debounceTimer)
  debounceTimer = setTimeout(()=>{
    handleSearch(searchInput.value.trim())
  },600)
})


// Optional: render all on load
renderResults(fruits);
